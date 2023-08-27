#pragma once
#include Common.glsl
#include Encoding.glsl

layout(rgba32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_Reservoirs;
struct Reservoir { float3 position; float3 normal; float3 radiance; float targetPdf; float weightSum; uint M; };
struct ReservoirPacked { uint4 data0; uint4 data1; };
#define pk_Reservoir_Zero Reservoir(0.0f.xxx, 0.0f.xxx, 0.0f.xxx, 0.0f, 0.0f, 0u)
#define pk_PackedReservoir_Zero ReservoirPacked(uint4(0), uint4(0))
#define RESTIR_LAYER_CUR 0
#define RESTIR_LAYER_PRE 2
#define RESTIR_LAYER_HIT 4
#define RESTIR_NORMAL_THRESHOLD 0.6f
#define RESTIR_SAMPLES_SPATIAL 6
#define RESTIR_SAMPLES_TEMPORAL 1

#if defined(PK_GI_CHECKERBOARD_TRACE)
    // only apply horizontal bias if using checkerboarding.
    // Vertical bias causes drifting due to data alignment.
    #define RESTIR_TEXEL_BIAS float2(0.49f, 0.0f)
#else
    #define RESTIR_TEXEL_BIAS 0.49f.xx
#endif

float Restir_GetSampleWeight(const Reservoir r) { return safePositiveRcp(r.targetPdf) * (r.weightSum / max(1, r.M)); }
float Restir_GetTargetPdf(const Reservoir r) { return dot(pk_Luminance.xyz, r.radiance); }
float Restir_GetJacobian(const float3 posCenter, const float3 posSample, const Reservoir r)
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

void Restir_Normalize(inout Reservoir r, uint maxM)
{
    r.weightSum /= max(r.M, 1);
    r.M = clamp(r.M, 0, maxM);
    r.weightSum *= r.M;
}

void Restir_CombineReservoir(inout Reservoir combined, const Reservoir b, float targetPdf, float rnd)
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

bool Restir_CombineReservoir(inout Reservoir combined, const Reservoir b, float rnd)
{
    combined.weightSum += b.weightSum;
    combined.M += b.M;

    if (rnd * combined.weightSum < b.weightSum)
    {
        combined.position = b.position;
        combined.normal = b.normal;
        combined.radiance = b.radiance;
        combined.targetPdf = b.targetPdf;
        return true;
    }

    return false;
}


ReservoirPacked Restir_Pack(const Reservoir r)
{
    ReservoirPacked o;
    o.data0.x = floatBitsToUint(r.position.x);
    o.data0.y = floatBitsToUint(r.position.y);
    o.data0.z = floatBitsToUint(r.position.z);
    o.data0.w = EncodeOctaUV(r.normal);
    o.data1.x = EncodeE5BGR9(r.radiance);
    o.data1.y = floatBitsToUint(r.targetPdf);
    o.data1.z = floatBitsToUint(r.weightSum);
    o.data1.w = r.M;
    return o;
}

ReservoirPacked Restir_Pack(const Reservoir r, uint packedNormal)
{
    ReservoirPacked o;
    o.data0.x = floatBitsToUint(r.position.x);
    o.data0.y = floatBitsToUint(r.position.y);
    o.data0.z = floatBitsToUint(r.position.z);
    o.data0.w = packedNormal;
    o.data1.x = EncodeE5BGR9(r.radiance);
    o.data1.y = floatBitsToUint(r.targetPdf);
    o.data1.z = floatBitsToUint(r.weightSum);
    o.data1.w = r.M;
    return o;
}

Reservoir Restir_Unpack(const ReservoirPacked packed)
{
    Reservoir r;
    r.position.x = uintBitsToFloat(packed.data0.x);
    r.position.y = uintBitsToFloat(packed.data0.y);
    r.position.z = uintBitsToFloat(packed.data0.z);
    r.normal = DecodeOctaUV(packed.data0.w);
    r.radiance = DecodeE5BGR9(packed.data1.x);
    r.targetPdf = uintBitsToFloat(packed.data1.y);
    r.weightSum = uintBitsToFloat(packed.data1.z); 
    r.M = packed.data1.w;
    return r;
}

ReservoirPacked Restir_Load_Packed(const int2 coord, const int layer)
{
    ReservoirPacked o;
    o.data0 = imageLoad(pk_Reservoirs, int3(coord, layer + 0));
    o.data1 = imageLoad(pk_Reservoirs, int3(coord, layer + 1));
    return o;
}

void Restir_Store_Packed(const int2 coord, const int layer, const ReservoirPacked packed)
{
    imageStore(pk_Reservoirs, int3(coord, layer + 0), packed.data0);
    imageStore(pk_Reservoirs, int3(coord, layer + 1), packed.data1);
}

void Restir_Store(const int2 coord, const int layer, const Reservoir r)
{
    const bool isValid = !isinf(r.weightSum) && !isnan(r.weightSum) && r.weightSum >= 0.0;
    Restir_Store_Packed(coord, layer, isValid ? Restir_Pack(r) : pk_PackedReservoir_Zero);
}

Reservoir Restir_Load(const int2 coord, const int layer) 
{
    const bool isValid = All_InArea(coord, int2(0), imageSize(pk_Reservoirs).xy);
    return isValid ? Restir_Unpack(Restir_Load_Packed(coord, layer)) : pk_Reservoir_Zero;
}

void Restir_Store_Hit(const int2 coord, const float3 position, const uint packedNormal, const float3 radiance)
{
    Reservoir r = Reservoir(position, 0.0f.xxx, radiance, 0.0f, 0.0f, 1u);
    Restir_Store_Packed(coord, RESTIR_LAYER_HIT, Restir_Pack(r, packedNormal));

    // Also copy cur to prev so that we can do temporal pass as expected.
    ReservoirPacked p = pk_FrameIndex.y == 0u ? pk_PackedReservoir_Zero : Restir_Load_Packed(coord, RESTIR_LAYER_CUR);
    Restir_Store_Packed(coord, RESTIR_LAYER_PRE, p);
}

Reservoir Restir_Load_HitAsReservoir(const int2 coord, const float3 origin, const float3 normal)
{
    Reservoir r = Restir_Load(coord, RESTIR_LAYER_HIT);
    r.targetPdf = Restir_GetTargetPdf(r);
    r.weightSum = r.targetPdf * PK_PI * safePositiveRcp(dot(normal, normalize(r.position - origin)));
    return r;
}
