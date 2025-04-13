#pragma once
#include "Common.glsl"
#include "Encoding.glsl"
#include "MortonOrder.glsl"

PK_DECLARE_SET_DRAW uniform uimage2DArray pk_Reservoirs0;
PK_DECLARE_SET_DRAW uniform uimage2DArray pk_Reservoirs1;

struct Reservoir 
{ 
    float3 position; 
    float3 normal; 
    float3 radiance; 
    float target_pdf; 
    float weight_sum; 
    uint M;
};

#define RESTIR_RESERVOIR_ZERO Reservoir(0.0f.xxx, 0.0f.xxx, 0.0f.xxx, 0.0f, 0.0f, 0u)
#define RESTIR_LAYER_CUR int(( pk_FrameIndex.y & 0x1u))
#define RESTIR_LAYER_PRE int((~pk_FrameIndex.y & 0x1u))
#define RESITR_NEARFIELD 0.05f
#define RESTIR_NORMAL_THRESHOLD 0.6f
#define RESTIR_SAMPLES_TEMPORAL 6
#define RESTIR_SAMPLES_SPATIAL 6
#define RESTIR_VALIDATION_ERROR_DIST 0.1f // 10%
#define RESTIR_VALIDATION_ERROR_LUMA 0.25f // 0.25 BT709 Log Luma
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
uint ReSTIR_GetSeed(int2 coord_base) { return ZCurveToIndex2D(coord_base) * RESTIR_SEED_STRIDE + pk_FrameRandom.x; }

int2 ReSTIR_PermutationSampling(int2 coord, bool mask)
{
    int2 offset = int2(pk_FrameRandom.y & 3, (pk_FrameRandom.y >> 2) & 3);
    return lerp(coord, ((coord + offset) ^ 3) - offset, mask.xx);
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
    const float random = make_unorm(hash);
    const float range = RESITR_NEARFIELD * depth;
    const float3 vec = origin - r.position;
    return (dot(vec, vec) / pow2(range)) < random;
}

float ReSTIR_GetTargetPdf(const Reservoir r) 
{ 
    return dot(PK_LUMA_BT709, r.radiance); 
}

float ReSTIR_GetSampleWeight(const Reservoir r, const float3 n, const float3 d) 
{ 
    return safePositiveRcp(r.target_pdf) * (r.weight_sum / max(1, r.M)); 
}

float ReSTIR_GetJacobian(const float3 pos_center, const float3 pos_sample, const Reservoir r)
{
    const float4 vec_center = normalizeLength(pos_center - r.position);
    const float4 vec_sample = normalizeLength(pos_sample - r.position);
    const float cos_senter = saturate(dot(r.normal, vec_center.xyz));
    const float cos_sample = saturate(dot(r.normal, vec_sample.xyz));
    const float jacobian = (cos_senter * pow2(vec_sample.w)) / (cos_sample * pow2(vec_center.w));
    return lerp(jacobian, 0.0f, isinf(jacobian) || isnan(jacobian));
}

float ReSTIR_GetTargetPdfNewSurf(const float3 pos_center, const float3 nor_center, const float3 pos_sample, const Reservoir r)
{
    const float3 direction_sample = normalize(r.position - pos_center);
    return ReSTIR_GetTargetPdf(r) *
           ReSTIR_GetJacobian(pos_center, pos_sample, r) *
           // @TODO Why was this here again. seems diffuse specific. 
           // Significantly reduces noise but introduces some bias. hmm.
           PK_PI * max(0.0f, dot(nor_center, direction_sample));
}


void ReSTIR_Normalize(inout Reservoir r, uint max_m)
{
    r.weight_sum /= max(r.M, 1);
    r.M = min(r.M, max_m);
    r.weight_sum *= r.M;
}

void ReSTIR_CombineReservoir(inout Reservoir combined, const Reservoir b, float target_pdf, uint hash)
{
    const float random = make_unorm(hash);
    const float weight = target_pdf * safePositiveRcp(b.target_pdf) * b.weight_sum;
    
    combined.weight_sum += weight;
    combined.M += lerp(0u, b.M, target_pdf > 0.0f);

    if (random * combined.weight_sum < weight)
    {
        combined.position = b.position;
        combined.normal = b.normal;
        combined.radiance = b.radiance;
        combined.target_pdf = target_pdf;
    }
}

void ReSTIR_CombineReservoirSimple(inout Reservoir combined, const Reservoir b, uint hash)
{
    const float random = make_unorm(hash);
    combined.weight_sum += b.weight_sum;
    combined.M += b.M; 

    if (random * combined.weight_sum < b.weight_sum)
    {
        combined.position = b.position;
        combined.normal = b.normal;
        combined.radiance = b.radiance;
        combined.target_pdf = b.target_pdf;
    }
}

void ReSTIR_StoreZero(const int2 coord)
{
    imageStore(pk_Reservoirs0, int3(coord, RESTIR_LAYER_PRE), uint4(0));
    imageStore(pk_Reservoirs1, int3(coord, RESTIR_LAYER_PRE), uint4(0));
}

void ReSTIR_Store_Current(const int2 coord, const Reservoir r)
{
    uint4 packed0;
    const float3 view_pos = WorldToViewPos(r.position);
    packed0.x = packHalf2x16(view_pos.xy);
    packed0.y = floatBitsToUint(view_pos.z);
    packed0.z = EncodeOctaUv2x16(r.normal);
    packed0.w = EncodeE5BGR9(r.radiance);
    imageStore(pk_Reservoirs0, int3(coord, RESTIR_LAYER_CUR), packed0);

    uint2 packed1;
    packed1.x = floatBitsToUint(r.weight_sum);
    packed1.y = packHalf2x16(r.target_pdf.xx) & 0xFFFFu;
    packed1.y |= r.M << 16u; 
    imageStore(pk_Reservoirs1, int3(coord, RESTIR_LAYER_CUR), packed1.xyxy);
}

Reservoir ReSTIR_Load_Previous(const int2 coord) 
{
    Reservoir r;

    const uint4 packed0 = imageLoad(pk_Reservoirs0, int3(coord, RESTIR_LAYER_PRE));
    const float3 view_pos = float3(unpackHalf2x16(packed0.x), uintBitsToFloat(packed0.y));
    r.position = ViewToWorldPosPrev(view_pos);
    r.normal = DecodeOctaUv2x16(packed0.z);
    r.radiance = DecodeE5BGR9(packed0.w);

    const uint2 packed1 = imageLoad(pk_Reservoirs1, int3(coord, RESTIR_LAYER_PRE)).xy;
    r.weight_sum = uintBitsToFloat(packed1.x);
    r.target_pdf = unpackHalf2x16(packed1.y).x;
    r.M = packed1.y >> 16u;
    return r;
}

uint4 ReSTIR_Pack_Hit(const float3 direction, const float hit_t, const float inverse_pdf, const uint hit_normal, const float3 radiance)
{
    uint4 packed;
    packed.xy = packHalf4x16(float4(direction.xyz * hit_t, inverse_pdf));
    packed.z = hit_normal;
    packed.w = EncodeE5BGR9(radiance);
    return packed;
}

Reservoir ReSTIR_Unpack_Hit(const uint4 packed, const float3 origin)
{
    const float4 offset_invPdf = unpackHalf4x16(packed.xy);
    Reservoir r = RESTIR_RESERVOIR_ZERO;
    r.position = origin + offset_invPdf.xyz;
    r.normal = DecodeOctaUv2x16(packed.z);
    r.radiance = DecodeE5BGR9(packed.w);
    r.target_pdf = ReSTIR_GetTargetPdf(r);
    r.weight_sum = r.target_pdf * offset_invPdf.w; 
    r.M = 1u;
    return r;
}
