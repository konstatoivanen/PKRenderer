#pragma once
#include Common.glsl
#include Encoding.glsl

layout(rgba32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_Reservoirs;
struct Reservoir { float3 position; float3 normal; float3 radiance; float targetPdf; float weightSum; uint M; uint age; };
#define pk_Reservoir_Zero Reservoir(0.0f.xxx, 0.0f.xxx, 0.0f.xxx, 0.0f, 0.0f, 0u, 0u)
#define RESTIR_LAYER_CUR 0
#define RESTIR_LAYER_PRE 2
#define RESTIR_LAYER_HIT 4
#define RESITR_NEARFIELD 0.5f
#define RESTIR_NORMAL_THRESHOLD 0.6f
#define RESTIR_SAMPLES_SPATIAL 6
#define RESTIR_SAMPLES_TEMPORAL 1
#define RESTIR_MAX_AGE 32

#if defined(PK_GI_CHECKERBOARD_TRACE)
    // Approximate bias due to checkerboard pattern
    // @TODO calculate this correctly
    #define RESTIR_TEXEL_BIAS float2(0.2f, 0.07f)
    #define RESTIR_TEMPORAL_RADIUS int2(2, 4)
#else
    #define RESTIR_TEXEL_BIAS 0.0f.xx
    #define RESTIR_TEMPORAL_RADIUS int2(2, 2)
#endif


bool ReSTIR_IsNearField(const float depth, const float3 origin, const Reservoir initial, float rnd)
{
    return distance(origin, initial.position) / (RESITR_NEARFIELD * log2(depth + 1.0f)) < rnd;
}

float ReSTIR_GetSampleWeight(const Reservoir r) { return safePositiveRcp(r.targetPdf) * (r.weightSum / max(1, r.M)); }
float ReSTIR_GetTargetPdf(const Reservoir r) { return dot(pk_Luminance.xyz, r.radiance); }

float ReSTIR_GetJacobian(const float3 posCenter, const float3 posSample, const Reservoir r)
{
    const float3 toCenter = posCenter - r.position;
    const float3 toSample = posSample - r.position;
    const float distCenter = length(toCenter);
    const float distSample = length(toSample);
    const float cosCenter = saturate(dot(r.normal, toCenter / distCenter));
    const float cosSample = saturate(dot(r.normal, toSample / distSample));
    const float jacobian = (cosCenter * pow2(distCenter)) / (cosSample * pow2(distSample));
    return isinf(jacobian) || isnan(jacobian) ? 0.0f : jacobian;
}

float ReSTIR_GetTargetPdfNewSurf(const float3 posCenter, const float3 normalCenter, const float3 posSample, const Reservoir r)
{
    const float3 directionSample = normalize(r.position - posCenter);
    return ReSTIR_GetTargetPdf(r) *
           ReSTIR_GetJacobian(posCenter, posSample, r) *
           PK_PI * max(0.0f, dot(normalCenter, directionSample));
}

int2 ReSTIR_CalculateTemporalResamplingOffset(int sampleIdx)
{
    sampleIdx &= 7;
    int mask2 = sampleIdx >> 1 & 0x01;
    int mask4 = 1 - (sampleIdx >> 2 & 0x01);
    int tmp0 = -1 + 2 * (sampleIdx & 0x01);
    int tmp1 = 1 - 2 * mask2;
    int tmp2 = mask4 | mask2;
    int tmp3 = mask4 | (1 - mask2);
    return int2(tmp0, tmp0 * tmp1) * int2(tmp2, tmp3) * RESTIR_TEMPORAL_RADIUS;
}

void ReSTIR_Normalize(inout Reservoir r, uint maxM)
{
    r.weightSum /= max(r.M, 1);
    r.M = clamp(r.M, 0, maxM);
    r.weightSum *= r.M;
}

void ReSTIR_CombineReservoir(inout Reservoir combined, const Reservoir b, float targetPdf, float rnd)
{
    float weight = targetPdf * safePositiveRcp(b.targetPdf) * b.weightSum;
    combined.weightSum += weight;
    combined.M += b.M;

    if (rnd * combined.weightSum < weight)
    {
        combined.position = b.position;
        combined.normal = b.normal;
        combined.radiance = b.radiance;
        combined.targetPdf = targetPdf;
    }
}

void ReSTIR_Store_Empty(const int2 coord, const int layer)
{
    imageStore(pk_Reservoirs, int3(coord, layer + 0), uint4(0));
    imageStore(pk_Reservoirs, int3(coord, layer + 1), uint4(0));
}

void ReSTIR_Store(const int2 coord, const int layer, const Reservoir r)
{
    const bool isValid = !isinf(r.weightSum) && !isnan(r.weightSum) && r.weightSum >= 0.0;
    uint4 packed0, packed1;
    packed0.xyz = floatBitsToUint(r.position);
    packed0.w = EncodeOctaUV(r.normal);
    packed1.x = EncodeE5BGR9(r.radiance);
    packed1.y = floatBitsToUint(r.targetPdf);
    packed1.z = floatBitsToUint(r.weightSum);
    packed1.w = ((r.age & 0xFFFFu) << 16u) | (r.M & 0xFFFFu);
    packed0 = isValid ? packed0 : uint4(0);
    packed1 = isValid ? packed1 : uint4(0);
    imageStore(pk_Reservoirs, int3(coord, layer + 0), packed0);
    imageStore(pk_Reservoirs, int3(coord, layer + 1), packed1);
}

Reservoir ReSTIR_Load(const int2 coord, const int layer) 
{
    const bool isValid = All_InArea(coord, int2(0), imageSize(pk_Reservoirs).xy);
    const uint4 packed0 = imageLoad(pk_Reservoirs, int3(coord, layer + 0));
    const uint4 packed1 = imageLoad(pk_Reservoirs, int3(coord, layer + 1));
    Reservoir r;
    r.position = uintBitsToFloat(packed0.xyz);
    r.normal = DecodeOctaUV(packed0.w);
    r.radiance = DecodeE5BGR9(packed1.x);
    r.targetPdf = uintBitsToFloat(packed1.y);
    r.weightSum = uintBitsToFloat(packed1.z); 
    r.M = packed1.w & 0xFFFF;
    r.age = (packed1.w >> 16u) & 0xFFFFu;
    return isValid ? r : pk_Reservoir_Zero;
}

void ReSTIR_Store_Hit(const int2 coord, const float3 direction, const float hitDist, const float3 normal, const uint hitNormal, const float3 radiance)
{
    const float invPdf = PK_PI * safePositiveRcp(dot(normal, direction));

    uint4 packed;
    packed.xy = packHalf4x16(float4(direction.xyz * hitDist, invPdf));
    packed.z = hitNormal;
    packed.w = EncodeE5BGR9(radiance);
    imageStore(pk_Reservoirs, int3(coord, RESTIR_LAYER_HIT), packed);
}

Reservoir ReSTIR_Load_HitAsReservoir(const int2 coord, const float3 origin)
{
    const uint4 packed = imageLoad(pk_Reservoirs, int3(coord, RESTIR_LAYER_HIT));
    const float4 offset_invPdf = unpackHalf4x16(packed.xy);
    Reservoir r = pk_Reservoir_Zero;
    r.position = origin + offset_invPdf.xyz;
    r.normal = DecodeOctaUV(packed.z);
    r.radiance = DecodeE5BGR9(packed.w);
    r.targetPdf = ReSTIR_GetTargetPdf(r);
    r.weightSum = r.targetPdf * offset_invPdf.w; 
    r.M = 1u;
    r.age = 0u;
    return All_InArea(coord, int2(0), imageSize(pk_Reservoirs).xy) ? r : pk_Reservoir_Zero;
}