#pragma once
#include Common.glsl
#include Encoding.glsl

layout(rgba32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_Reservoirs;
struct HitSample { float3 position; float3 normal; float3 radiance; };
struct Reservoir { HitSample selected; float targetPdf; float weightSum; uint M; };
struct ReservoirPacked { uint4 data0; uint4 data1; };
#define pk_HitSample_Zero HitSample(0.0f.xxx, 0.0f.xxx, 0.0f.xxx)
#define pk_Reservoir_Zero Reservoir(pk_HitSample_Zero, 0.0f, 0.0f, 0u)
#define pk_PackedReservoir_Zero ReservoirPacked(uint4(0), uint4(0))
#define RESTIR_LAYER_CUR 0
#define RESTIR_LAYER_PRE 2
#define RESTIR_LAYER_HIT 4
#define RESTIR_SAMPLES_SPATIAL 6
#define RESTIR_SAMPLES_TEMPORAL 1
#define RESTIR_RADIUS_TEMPORAL 2.0f
#define RESTIR_RADIUS_SPATIAL lerp(8.0f, 32.0f, saturate(pk_ScreenSize.y / 1080.0f)) 

float GetSelectedSampleWeight(const Reservoir r) { return safePositiveRcp(r.targetPdf) * (r.weightSum / max(1, r.M)); }
float GetSelectedTargetPdf(const Reservoir r) { return dot(pk_Luminance.xyz, r.selected.radiance); }

void NormalizeReservoir(inout Reservoir r, uint maxM)
{
    r.weightSum /= max(r.M, 1);
    r.M = clamp(r.M, 0, maxM);
    r.weightSum *= r.M;
}

bool UpdateCombinedReservoir(inout Reservoir combined, const Reservoir b, float rnd)
{
    combined.weightSum += b.weightSum;
    combined.M += b.M;

    if (rnd * combined.weightSum < b.weightSum)
    {
        combined.selected = b.selected;
        combined.targetPdf = b.targetPdf;
        return true;
    }

    return false;
}

void UpdateCombinedReservoir_NewSurf(inout Reservoir combined, const Reservoir b, float targetPdf_b, float rnd)
{
    float weight = targetPdf_b * safePositiveRcp(b.targetPdf) * b.weightSum;
    combined.weightSum += weight;
    combined.M += b.M;

    if (rnd * combined.weightSum < weight)
    {
        combined.selected = b.selected;
        combined.targetPdf = targetPdf_b;
    }
}

ReservoirPacked Restir_Pack(const Reservoir r)
{
    ReservoirPacked o;
    o.data0.x = floatBitsToUint(r.selected.position.x);
    o.data0.y = floatBitsToUint(r.selected.position.y);
    o.data0.z = floatBitsToUint(r.selected.position.z);
    o.data0.w = EncodeOctaUV(r.selected.normal);
    o.data1.x = EncodeE5BGR9(r.selected.radiance);
    o.data1.y = floatBitsToUint(r.targetPdf);
    o.data1.z = floatBitsToUint(r.weightSum);
    o.data1.w = r.M;
    return o;
}

Reservoir Restir_Unpack(const ReservoirPacked packed)
{
    Reservoir r;
    r.selected.position.x = uintBitsToFloat(packed.data0.x);
    r.selected.position.y = uintBitsToFloat(packed.data0.y);
    r.selected.position.z = uintBitsToFloat(packed.data0.z);
    r.selected.normal = DecodeOctaUV(packed.data0.w);
    r.selected.radiance = DecodeE5BGR9(packed.data1.x);
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

void Restir_Store_Hit(const int2 coord, const float3 position, const float3 normal, const float3 radiance, const float inversePdf)
{
    Reservoir r = Reservoir(HitSample(position, normal, radiance), dot(pk_Luminance.xyz, radiance), inversePdf, 1u);
    r.weightSum *= r.targetPdf;
    Restir_Store_Packed(coord, RESTIR_LAYER_HIT, Restir_Pack(r));
}

void Restir_CopyToPrev(const int2 coord)
{
    ReservoirPacked p = pk_FrameIndex.y == 0u ? pk_PackedReservoir_Zero : Restir_Load_Packed(coord, RESTIR_LAYER_PRE);
    Restir_Store_Packed(coord, RESTIR_LAYER_PRE, p);
}
