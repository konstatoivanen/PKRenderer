#pragma once
#include Common.glsl
#include Encoding.glsl
#include MortonOrder.glsl

layout(rgba32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_Reservoirs;
struct Reservoir { float3 position; float3 normal; float3 radiance; float targetPdf; float weightSum; uint M;};
#define pk_Reservoir_Zero Reservoir(0.0f.xxx, 0.0f.xxx, 0.0f.xxx, 0.0f, 0.0f, 0u)
#define RESTIR_LAYER_CUR int(( pk_FrameIndex.y & 0x1u) << 1u)
#define RESTIR_LAYER_PRE int((~pk_FrameIndex.y & 0x1u) << 1u)
#define RESITR_NEARFIELD 0.05f
#define RESTIR_NORMAL_THRESHOLD 0.6f
#define RESTIR_SAMPLES_TEMPORAL 6
#define RESTIR_SAMPLES_SPATIAL 6
#define RESTIR_MAX_M 20
#define RESTIR_SEED_STRIDE (RESTIR_SAMPLES_SPATIAL + 1)

#if defined(PK_GI_CHECKERBOARD_TRACE)
    // Approximate bias due to checkerboard pattern
    // @TODO calculate this correctly
    #define RESTIR_TEXEL_BIAS float2(0.2f, 0.07f)
    #define RESTIR_TEMPORAL_RADIUS int2(1, 2)
    #define RESTIR_SCREEN_SIZE_MINUSONE (int2(pk_ScreenSize.x / 2, pk_ScreenSize.y) - 1)
#else
    #define RESTIR_SCREEN_SIZE_MINUSONE (int2(pk_ScreenSize.xy) - 1)
    #define RESTIR_TEXEL_BIAS 0.0f.xx
    #define RESTIR_TEMPORAL_RADIUS int2(1, 1)
#endif

int2 ReSTIR_WrapCoord(int2 coord)
{
    const int2 size = RESTIR_SCREEN_SIZE_MINUSONE;
    return size - abs(size - abs(coord));
}

// Wellon Hash: https://nullprogram.com/blog/2018/07/31/
uint ReSTIR_Hash(uint seed)
{
    seed ^= seed >> 16;
    seed *= 0x7feb352dU;
    seed ^= seed >> 15;
    seed *= 0x846ca68bU;
    seed ^= seed >> 16;
    return seed;
}

// Stride seed by max number of contiguous offsets in restir pass
// Wellons hash works well for low entropy input. 
// By striding the seed we get different values for pixels while keeping low entropy input.
uint ReSTIR_GetSeed(int2 baseCoord) { return ZCurveToIndex2D(baseCoord) * RESTIR_SEED_STRIDE + pk_FrameRandom.x; }
float ReSTIR_ToUnorm(uint x) { return uintBitsToFloat(x & 0x007fffffu | 0x3f800000u) - 1.0; }

int2 ReSTIR_PermutationSampling(int2 coord, bool mask)
{
    int2 offset = int2(pk_FrameRandom.y & 3, (pk_FrameRandom.y >> 2) & 3);
    return mask ? ((coord + offset) ^ 3) - offset : coord;
}

int2 ReSTIR_GetTemporalResamplingCoord(const int2 coord, int hash, int scale, bool permute)
{
    hash &= 7;
    const int m2 = hash >> 1 & 0x01;
    const int m4 = 1 - (hash >> 2 & 0x01);
    const int t = -1 + 2 * (hash & 0x01);
    const int2 offset = int2(t, t * (1 - 2 * m2)) * int2(m4 | m2, m4 | (1 - m2)) * RESTIR_TEMPORAL_RADIUS * scale;
    const int2 scoord = ReSTIR_PermutationSampling(coord + offset, permute);
    return ReSTIR_WrapCoord(scoord);
}

int2 ReSTIR_GetSpatialResamplingCoord(const int2 coord, int scale, uint hash) 
{ 
    const uint w = 1u << uint(scale);
    const uint m = w - 1u;
    return ReSTIR_WrapCoord(coord + int2(hash & m, (hash >> 8u) & m) - int2(w / 2));
}

bool ReSTIR_NearFieldReject(const float depth, const float3 origin, const Reservoir r, uint hash)
{
    const float random = ReSTIR_ToUnorm(hash);
    const float range = RESITR_NEARFIELD * depth;
    const float3 vec = origin - r.position;
    return (dot(vec, vec) / pow2(range)) < random;
}

float ReSTIR_GetTargetPdf(const Reservoir r) { return dot(pk_Luminance.xyz, r.radiance); }
float ReSTIR_GetSampleWeight(const Reservoir r, const float3 n, const float3 d) 
{ 
    return safePositiveRcp(r.targetPdf) * (r.weightSum / max(1, r.M)) * dot(n, d) * PK_INV_PI; 
}

float ReSTIR_GetJacobian(const float3 posCenter, const float3 posSample, const Reservoir r)
{
    const float4 centervec = normalizeLength(posCenter - r.position);
    const float4 samplevec = normalizeLength(posSample - r.position);
    const float cosCenter = saturate(dot(r.normal, centervec.xyz));
    const float cosSample = saturate(dot(r.normal, samplevec.xyz));
    const float jacobian = (cosCenter * pow2(centervec.w)) / (cosSample * pow2(samplevec.w));
    return isinf(jacobian) || isnan(jacobian) ? 0.0f : jacobian;
}

float ReSTIR_GetTargetPdfNewSurf(const float3 posCenter, const float3 normalCenter, const float3 posSample, const Reservoir r)
{
    const float3 directionSample = normalize(r.position - posCenter);
    return ReSTIR_GetTargetPdf(r) *
           ReSTIR_GetJacobian(posCenter, posSample, r) *
           PK_PI * max(0.0f, dot(normalCenter, directionSample));
}


void ReSTIR_Normalize(inout Reservoir r, uint maxM)
{
    r.weightSum /= max(r.M, 1);
    r.M = min(r.M, maxM);
    r.weightSum *= r.M;
}

void ReSTIR_CombineReservoir(inout Reservoir combined, const Reservoir b, float targetPdf, uint hash)
{
    const float random = ReSTIR_ToUnorm(hash);
    const float weight = targetPdf * safePositiveRcp(b.targetPdf) * b.weightSum;
    
    combined.weightSum += weight;
    combined.M += uint(targetPdf > 0.0f) * b.M; 

    if (random * combined.weightSum < weight)
    {
        combined.position = b.position;
        combined.normal = b.normal;
        combined.radiance = b.radiance;
        combined.targetPdf = targetPdf;
    }
}

void ReSTIR_CombineReservoirSimple(inout Reservoir combined, const Reservoir b, uint hash)
{
    const float random = ReSTIR_ToUnorm(hash);
    combined.weightSum += b.weightSum;
    combined.M += b.M; 

    if (random * combined.weightSum < b.weightSum)
    {
        combined.position = b.position;
        combined.normal = b.normal;
        combined.radiance = b.radiance;
        combined.targetPdf = b.targetPdf;
    }
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
    packed1.w = r.M;
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
    r.M = packed1.w;
    return isValid ? r : pk_Reservoir_Zero;
}

uint4 ReSTIR_Pack_Hit(const float3 direction, const float hitDist, const float3 normal, const uint hitNormal, const float3 radiance)
{
    const float invPdf = PK_PI * safePositiveRcp(dot(normal, direction));
    uint4 packed;
    packed.xy = packHalf4x16(float4(direction.xyz * hitDist, invPdf));
    packed.z = hitNormal;
    packed.w = EncodeE5BGR9(radiance);
    return packed;
}

Reservoir ReSTIR_Unpack_Hit(const uint4 packed, const float3 origin)
{
    const float4 offset_invPdf = unpackHalf4x16(packed.xy);
    Reservoir r = pk_Reservoir_Zero;
    r.position = origin + offset_invPdf.xyz;
    r.normal = DecodeOctaUV(packed.z);
    r.radiance = DecodeE5BGR9(packed.w);
    r.targetPdf = ReSTIR_GetTargetPdf(r);
    r.weightSum = r.targetPdf * offset_invPdf.w; 
    r.M = 1u;
    return r;
}
