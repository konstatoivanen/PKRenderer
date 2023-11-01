#pragma once
#include BRDF.glsl
#include Encoding.glsl
#include SHL1.glsl

#ifndef PK_GI_LOAD_LVL
#define PK_GI_LOAD_LVL 0
#endif

#ifndef PK_GI_STORE_LVL
#define PK_GI_STORE_LVL 0
#endif

PK_DECLARE_CBUFFER(pk_GI_Parameters, PK_SET_SHADER)
{
    float4 pk_GI_VolumeST;
    uint4 pk_GI_VolumeSwizzle;
    uint2 pk_GI_RayDither;
    float pk_GI_VoxelSize; 
};

layout(rgba32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_GI_PackedDiff;
layout(rg32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_GI_PackedSpec;
layout(r32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_GI_ResolvedWrite;
PK_DECLARE_SET_SHADER uniform texture2DArray pk_GI_ResolvedRead;

#define PK_GI_APPROX_ROUGH_SPEC 1
// Should surface shading approximate sheen & clear coat from diffuse sh
#define PK_GI_APPROX_ROUGH_SPEC_EXTRA 1
#define PK_GI_USE_BIASED_DEPTH 1

#define PK_GI_LVL_DIFF0 0
#define PK_GI_LVL_DIFF1 1
#define PK_GI_LVL_SPEC 2

#define PK_GI_MIN_ROUGH_SPEC 0.35f
#define PK_GI_MAX_ROUGH_SPEC 0.45f
#define PK_GI_AO_DIFF_POWER 0.125f
#define PK_GI_AO_SPEC_POWER 0.05f

#define PK_GI_RAY_TMAX 100.0f

#define PK_GI_MIN_ACCUM 0.03f
#define PK_GI_DIFF_MAX_HISTORY 32u
#define PK_GI_SPEC_MAX_HISTORY 128u
#define PK_GI_SPEC_ANTILAG_BASE_POWER 0.5f
#define PK_GI_SPEC_ANTILAG_CURVE 0.66f
#define PK_GI_SPEC_ANTILAG_MIN 0.03f
#define PK_GI_SPEC_ANTILAG_MAX 1.0f
#define PK_GI_MAX_LUMA_GAIN 0.5f
#define PK_GI_DISK_FILTER_RADIUS 3.0f

#if PK_GI_USE_BIASED_DEPTH == 1
#define PK_GI_SAMPLE_CLIP_DEPTH(uv) SampleClipDepthBiased(uv)
#define PK_GI_SAMPLE_DEPTH(coord) SampleViewDepthBiased(coord)
#define PK_GI_GATHER_PREV_DEPTH(uv) GatherPreviousViewDepthsBiased(uv)
#define PK_GI_SAMPLE_PREV_DEPTH(coord) SamplePreviousViewDepthBiased(coord)
#else
#define PK_GI_SAMPLE_CLIP_DEPTH(uv) SampleClipDepth(uv)
#define PK_GI_SAMPLE_DEPTH(coord) SampleViewDepth(coord)
#define PK_GI_GATHER_PREV_DEPTH(uv) GatherPreviousViewDepths(uv)
#define PK_GI_SAMPLE_PREV_DEPTH(coord) SamplePreviousViewDepth(coord)
#endif

//----------STRUCTS----------//
struct GIDiff { SH sh; float ao; float history; };
struct GISpec { float3 radiance; float ao; float history; };
#define PK_GI_DIFF_ZERO GIDiff(pk_ZeroSH, 0.0f, 0.0f)
#define PK_GI_SPEC_ZERO GISpec(0.0f.xxx, 0.0f, 0.0f)

//----------UTILITIES----------//
float GI_Alpha(const GIDiff a) { return max(1.0f / (floor(a.history) + 1.0f), PK_GI_MIN_ACCUM); }
float GI_Alpha(const GISpec a) { return max(1.0f / (floor(a.history) + 1.0f), PK_GI_MIN_ACCUM); }
GIDiff GI_Sum(const GIDiff a, const GIDiff b, const float w) { return GIDiff(SH_Add(a.sh, b.sh, w), a.ao + b.ao * w, a.history + b.history * w); }
GISpec GI_Sum(const GISpec a, const GISpec b, const float w) { return GISpec(a.radiance + b.radiance * w, a.ao + b.ao * w, a.history + b.history * w); }
GIDiff GI_Sum_NoHistory(const GIDiff a, const GIDiff b, const float w) { return GIDiff(SH_Add(a.sh, b.sh, w), a.ao + b.ao * w, a.history); }
GISpec GI_Sum_NoHistory(const GISpec a, const GISpec b, const float w) { return GISpec(a.radiance + b.radiance * w, a.ao + b.ao * w, a.history); }
GIDiff GI_Mul_NoHistory(const GIDiff a, const float w) { return GIDiff(SH_Scale(a.sh, w), a.ao * w, a.history); }
GISpec GI_Mul_NoHistory(const GISpec a, const float w) { return GISpec(a.radiance * w, a.ao * w, a.history); }
GIDiff GI_Interpolate(const GIDiff a, const GIDiff b, const float w) { return GIDiff(SH_Interpolate(a.sh, b.sh, w), lerp(a.ao, b.ao, w), a.history); }
GISpec GI_Interpolate(const GISpec a, const GISpec b, const float w) { return GISpec(lerp(a.radiance, b.radiance, w), lerp(a.ao, b.ao, w), a.history); }
float GI_Luminance(const GIDiff a) { return SH_ToLuminanceL0(a.sh); }
float GI_Luminance(const GISpec a) { return dot(PK_LUMA_BT709, a.radiance); }
float GI_LogLuminance(const GIDiff a) { return log(1.0f + GI_Luminance(a)); }
float GI_LogLuminance(const GISpec a) { return log(1.0f + GI_Luminance(a)); }
float GI_MaxLuma(const GIDiff a, float alpha) { return GI_Luminance(a) + (PK_GI_MAX_LUMA_GAIN / (1.0f - alpha)); }
float GI_MaxLuma(const GISpec a, float alpha) { return GI_Luminance(a) + (PK_GI_MAX_LUMA_GAIN / (1.0f - alpha)); }
float GI_LumaScale(float luma, float maxLuma) { return (min(luma, maxLuma) + 1e-6f) / (luma + 1e-6f); }
GIDiff GI_ClampLuma(GIDiff a, float maxLuma) { return GIDiff(SH_Scale(a.sh, GI_LumaScale(GI_Luminance(a), maxLuma)), a.ao, a.history); }
GISpec GI_ClampLuma(GISpec a, float maxLuma) { return GISpec(a.radiance * GI_LumaScale(GI_Luminance(a), maxLuma), a.ao, a.history); }
float GI_RoughSpecWeight(float roughness) { return smoothstep(PK_GI_MIN_ROUGH_SPEC, PK_GI_MAX_ROUGH_SPEC, roughness); }

uint GI_GetCheckerboardOffset(uint2 coord, uint frame) { return ((coord.x ^ coord.y) ^ frame) & 0x1u; }

int2 GI_ExpandCheckerboardCoord(uint2 coord, uint offset)
{
#if defined(PK_GI_CHECKERBOARD_TRACE)
    coord.x *= 2;
    coord.x += GI_GetCheckerboardOffset(coord, pk_FrameIndex.y + offset);
#endif
    return int2(coord);
}

int2 GI_CollapseCheckerboardCoord(const float2 screenUV, const uint offset)
{
    int2 coord = int2(screenUV);
#if defined(PK_GI_CHECKERBOARD_TRACE)
    float2 ddxy = (screenUV - coord) - 0.5f.xx;
    int am = int(step(ddxy.x, ddxy.y));
    int2 xy = int2(am, 1 - am) * int2(sign(ddxy));
    coord += xy * int(GI_GetCheckerboardOffset(uint2(coord), pk_FrameIndex.y + offset));
    coord.x /= 2;
#endif
    return coord;
}

int2 GI_ExpandCheckerboardCoord(uint2 coord) { return GI_ExpandCheckerboardCoord(coord, 0u); }

GISpec GI_ShadeRoughSpecular(const float3 normal, const float3 viewdir, const float roughness, const GIDiff diff)
{
    float directionality;
    float3 direction = SH_ToPrimeDir(diff.sh, directionality);
    
    direction = WorldToViewDir(direction);
    directionality = saturate(directionality * 0.666f);

    // Remap roughness if lighting is uniform over hemisphere
    const float newRoughness = lerp(1.0f, roughness, directionality);
    const float3 specular = SH_ToColor(diff.sh) * EvaluateBxDF_Specular(normal, -viewdir, newRoughness, direction) * PK_PI;

    return GISpec(specular, diff.ao, diff.history);
}

float3 GI_ShadeRoughSpecularDetails(BxDFSurf surf, const GIDiff diff)
{
    float directionality;
    float3 direction = SH_ToPrimeDir(diff.sh, directionality);
    
    direction = WorldToViewDir(direction);
    directionality = saturate(directionality * 0.666f);

    // Remap clearcoat if lighting is uniform over hemisphere
    surf.clearCoatGloss *= directionality;

    return EvaluateBxDF_SpecularExtra(surf, direction, SH_ToColor(diff.sh));
}

//----------PACK / UNPACK FUNCTIONS----------//
uint4 GI_Pack_Diff(const GIDiff u) { return uint4(packHalf4x16(u.sh.Y), packHalf4x16(float4(u.sh.CoCg, u.ao, u.history))); }
uint2 GI_Pack_Spec(const GISpec u) { return uint2(EncodeE5BGR9(u.radiance), packHalf2x16(float2(u.ao, u.history))); }

GIDiff GI_Unpack_Diff(const uint4 p) 
{
    const float4 v0 = unpackHalf4x16(p.xy);
    const float4 v1 = unpackHalf4x16(p.zw);
    return GIDiff(SH(v0, v1.xy), v1.z, v1.w);
}

GISpec GI_Unpack_Spec(const uint2 p) 
{ 
    const float2 v = unpackHalf2x16(p.y);
    return GISpec(DecodeE5BGR9(p.x), v.x, v.y);
}

//----------LOAD/STORE FUNCTIONS----------//
uint4 GI_Load_Packed_Diff(const int2 coord, const int l) { return imageLoad(pk_GI_PackedDiff, int3(coord, l)); }
uint2 GI_Load_Packed_Spec(const int2 coord, const int l) { return imageLoad(pk_GI_PackedSpec, int3(coord, l)).xy; }
uint4 GI_Load_Packed_Diff(const int2 coord) { return GI_Load_Packed_Diff(coord, PK_GI_LOAD_LVL); }
uint2 GI_Load_Packed_Spec(const int2 coord) { return GI_Load_Packed_Spec(coord, PK_GI_LOAD_LVL); }
GIDiff GI_Load_Diff(const int2 coord, int l) { return GI_Unpack_Diff(GI_Load_Packed_Diff(coord, l)); }
GISpec GI_Load_Spec(const int2 coord, int l) { return GI_Unpack_Spec(GI_Load_Packed_Spec(coord, l)); }
GIDiff GI_Load_Diff(const int2 coord) { return GI_Unpack_Diff(GI_Load_Packed_Diff(coord)); }
GISpec GI_Load_Spec(const int2 coord) { return GI_Unpack_Spec(GI_Load_Packed_Spec(coord)); }
void GI_Store_Packed_Diff(const int2 coord, const int l, const uint4 p) { imageStore(pk_GI_PackedDiff, int3(coord, l), p); }
void GI_Store_Packed_Spec(const int2 coord, const int l, const uint2 p) { imageStore(pk_GI_PackedSpec, int3(coord, l), p.xyxy); }
void GI_Store_Packed_Diff(const int2 coord, const uint4 p) { GI_Store_Packed_Diff(coord, PK_GI_STORE_LVL, p); }
void GI_Store_Packed_Spec(const int2 coord, const uint2 p) { GI_Store_Packed_Spec(coord, PK_GI_STORE_LVL, p); }
void GI_Store_Diff(const int2 coord, const int l, const GIDiff u) { GI_Store_Packed_Diff(coord, l, GI_Pack_Diff(u)); }
void GI_Store_Spec(const int2 coord, const int l, const GISpec u) { GI_Store_Packed_Spec(coord, l, GI_Pack_Spec(u)); }
void GI_Store_Diff(const int2 coord, const GIDiff u) { GI_Store_Packed_Diff(coord, GI_Pack_Diff(u)); }
void GI_Store_Spec(const int2 coord, const GISpec u) { GI_Store_Packed_Spec(coord, GI_Pack_Spec(u)); }

float3 GI_Load_Resolved_Diff(const float2 uv) { return texelFetch(pk_GI_ResolvedRead, int3(uv * pk_ScreenSize.xy, 0), 0).xyz; }
float3 GI_Load_Resolved_Spec(const float2 uv) { return texelFetch(pk_GI_ResolvedRead, int3(uv * pk_ScreenSize.xy, 1), 0).xyz; }

void GI_Store_Resolved_Diff(const int2 coord, const float3 N, const GIDiff diff)
{
    const float3 radiance = SH_ToIrradiance(diff.sh, N) * pow(diff.ao, PK_GI_AO_DIFF_POWER);
    imageStore(pk_GI_ResolvedWrite, int3(coord, 0), EncodeE5BGR9(radiance).xxxx);
}

void GI_Store_Resolved_Spec(const int2 coord, const GISpec spec)
{
    const float3 radiance = spec.radiance * pow(spec.ao, PK_GI_AO_SPEC_POWER);
    imageStore(pk_GI_ResolvedWrite, int3(coord, 1), EncodeE5BGR9(radiance).xxxx);
}

float3 GI_ShadeRoughSpecularDetails(const BxDFSurf surf, const float2 uv)
{
    #if PK_GI_APPROX_ROUGH_SPEC_EXTRA == 1
    const GIDiff diff = GI_Load_Diff(int2(uv * pk_ScreenSize.xy), 1);
    return GI_ShadeRoughSpecularDetails(surf, diff);
    #else
    return 0.0f.xxx;
    #endif
}