#pragma once
#include Utilities.glsl
#include SampleDistribution.glsl
#include BlueNoise.glsl
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
    int4 pk_GI_Checkerboard_Offset;
    uint2 pk_GI_RayDither;
    float pk_GI_VoxelSize; 
    float pk_GI_ChromaBias; 
};

layout(rg32ui, set = PK_SET_SHADER) uniform uimage2D pk_GI_RayHits;
layout(rgba32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_GI_PackedDiff;
layout(rg32ui, set = PK_SET_SHADER) uniform uimage2DArray pk_GI_PackedSpec;
layout(r8ui, set = PK_SET_SHADER) uniform uimage2D pk_GI_ScreenDataMipMask; 
layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_GI_VolumeMaskWrite;
layout(rgba16f, set = PK_SET_SHADER) uniform image3D pk_GI_VolumeWrite;
PK_DECLARE_SET_SHADER uniform usampler2DArray pk_GI_ScreenDataMips;
PK_DECLARE_SET_SHADER uniform sampler3D pk_GI_VolumeRead;

#define PK_GI_APPROX_ROUGH_SPEC 1

#define PK_GI_LVL_DIFF0 0
#define PK_GI_LVL_DIFF1 1
#define PK_GI_LVL_SPEC 2

#define PK_GI_VX_MIP_COUNT 7
#define PK_GI_VX_MIN_HISTORY 4.0f
#define PK_GI_VX_CONE_SIZE 0.25f
#define PK_GI_MIN_ROUGH_SPEC 0.35f
#define PK_GI_MAX_ROUGH_SPEC 0.45f
#define PK_GI_AO_DIFF_POWER 0.125f
#define PK_GI_AO_SPEC_POWER 0.05f

#define PK_GI_RAY_TMAX 100.0f
#define PK_GI_MIN_ACCUM 0.05f
#define PK_GI_DIFF_MAX_HISTORY 32u
#define PK_GI_SPEC_MAX_HISTORY 128u
#define PK_GI_SPEC_ANTILAG_BASE_POWER 0.5f
#define PK_GI_SPEC_ANTILAG_CURVE 0.66f
#define PK_GI_SPEC_ANTILAG_MIN 0.03f
#define PK_GI_SPEC_ANTILAG_MAX 1.0f
#define PK_GI_MAX_LUMA_GAIN 0.5f
#define PK_GI_HISTORY_FILL_THRESHOLD 8
#define PK_GI_DISK_FILTER_RADIUS 3.0f

#define PK_GI_DATA_LOAD_MIP(c, l, m) texelFetch(pk_GI_ScreenDataMips, int3(c, l), m).xy

//----------STRUCTS----------//
struct GIDiff { SH sh; float ao; float history; };
struct GISpec { float3 radiance; float ao; float history; };
struct GIRayHit { float dist; bool isMiss; bool isScreen; };
struct GIRayHits { GIRayHit diff; GIRayHit spec; uint diffNormal; };
struct GIRayParams { float3 origin; float3 normal; float3 diffdir; float3 specdir; float roughness; };
#define pk_Zero_GIDiff GIDiff(pk_ZeroSH, 0.0f, 0.0f)
#define pk_Zero_GISpec GISpec(0.0f.xxx, 0.0f, 0.0f)

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
float GI_Luminance(const GISpec a) { return dot(pk_Luminance.rgb, a.radiance); }
float GI_LogLuminance(const GIDiff a) { return log(1.0f + GI_Luminance(a)); }
float GI_LogLuminance(const GISpec a) { return log(1.0f + GI_Luminance(a)); }
float GI_MaxLuma(const GIDiff a, float alpha) { return GI_Luminance(a) + (PK_GI_MAX_LUMA_GAIN / (1.0f - alpha)); }
float GI_MaxLuma(const GISpec a, float alpha) { return GI_Luminance(a) + (PK_GI_MAX_LUMA_GAIN / (1.0f - alpha)); }
float GI_LumaScale(float luma, float maxLuma) { return (min(luma, maxLuma) + 1e-6f) / (luma + 1e-6f); }
GIDiff GI_ClampLuma(GIDiff a, float maxLuma) { return GIDiff(SH_Scale(a.sh, GI_LumaScale(GI_Luminance(a), maxLuma)), a.ao, a.history); }
GISpec GI_ClampLuma(GISpec a, float maxLuma) { return GISpec(a.radiance * GI_LumaScale(GI_Luminance(a), maxLuma), a.ao, a.history); }

#define GI_GET_RAY_PARAMS(COORD, RAYCOORD, DEPTH, OUT_PARAMS)                                                                   \
{                                                                                                                               \
    const float4 normalRoughness = SampleWorldNormalRoughness(COORD);                                                           \
    const float3 normal = normalRoughness.xyz;                                                                                  \
    const float3 v = GlobalNoiseBlue(RAYCOORD + pk_GI_RayDither, pk_FrameIndex.y);                                              \
    const float2 Xi = saturate(v.xy + ((v.z - 0.5f) / 256.0f));                                                                 \
    /* Apply bias to avoid rays clipping with geo at high distances */                                                          \
    float3 origin = SampleWorldPosition(COORD, DEPTH - DEPTH * 1e-2f);                                                          \
    float3 viewdir = normalize(origin - pk_WorldSpaceCameraPos.xyz);                                                            \
    /* Apply bias to avoid rays clipping with geo at high angles */                                                             \
    origin += normal * (0.01f / (saturate(dot(-viewdir, normal)) + 0.01f)) * 0.05f;                                             \
    OUT_PARAMS.origin = origin;                                                                                                 \
    OUT_PARAMS.normal = normal;                                                                                                 \
    OUT_PARAMS.diffdir = ImportanceSampleLambert(Xi, normal);                                                                   \
    OUT_PARAMS.specdir = ImportanceSampleSmithGGX(Xi.yx, normal, viewdir, normalRoughness.w);                                   \
    OUT_PARAMS.roughness = normalRoughness.w;                                                                                   \
}                                                                                                                               \

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

float3 GI_VoxelToWorldSpace(int3 coord) { return coord * pk_GI_VoxelSize + pk_GI_VolumeST.xyz + pk_GI_VoxelSize * 0.5f; }
int3   GI_WorldToVoxelSpace(float3 worldpos) { return int3((worldpos - pk_GI_VolumeST.xyz) * pk_GI_VolumeST.www); }
float3 GI_QuantizeWorldToVoxelSpace(float3 worldpos) { return GI_VoxelToWorldSpace(GI_WorldToVoxelSpace(worldpos)); }
float3 GI_WorldToVoxelUVW(float3 worldpos) { return ((worldpos - pk_GI_VolumeST.xyz) * pk_GI_VolumeST.www) / textureSize(pk_GI_VolumeRead, 0).xyz;  }
float3 GI_WorldToVoxelUVWDiscrete(float3 worldpos) { return (GI_WorldToVoxelSpace(worldpos) + 0.5f.xxx) / textureSize(pk_GI_VolumeRead, 0).xyz;  }
float3 GI_WorldToVoxelClipSpace(float3 worldpos) { return GI_WorldToVoxelUVW(worldpos) * 2.0f - 1.0f; }
float4 GI_WorldToVoxelNDCSpace(float3 worldpos) 
{ 
    float3 clippos = GI_WorldToVoxelClipSpace(worldpos);
    return float4(clippos[pk_GI_VolumeSwizzle.x], clippos[pk_GI_VolumeSwizzle.y], clippos[pk_GI_VolumeSwizzle.z] * 0.5f + 0.5f, 1);
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
uint4 GI_Load_Packed_Mip_Diff(const int2 coord, const int mip) { return uint4(PK_GI_DATA_LOAD_MIP(coord, PK_GI_LVL_DIFF0, mip), PK_GI_DATA_LOAD_MIP(coord, PK_GI_LVL_DIFF1, mip)); }
uint2 GI_Load_Packed_Mip_Spec(const int2 coord, const int mip) { return PK_GI_DATA_LOAD_MIP(coord, PK_GI_LVL_SPEC, mip); }
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

GIRayHits GI_Load_RayHits(const int2 coord)
{
    uint2 packed = imageLoad(pk_GI_RayHits, coord).xy;
    const bool isScreenDiff = bitfieldExtract(packed.x, 15, 1) != 0;
    const bool isScreenSpec = bitfieldExtract(packed.x, 31, 1) != 0;
    packed.x &= 0x7FFF7FFFu; // Remove sign bits
    const bool isMissDiff = bitfieldExtract(packed.x, 0, 16) == 0x7C00u;
    const bool isMissSpec = bitfieldExtract(packed.x, 16, 16) == 0x7C00u;
    const float2 hitDist = unpackHalf2x16(packed.x);
    return GIRayHits(GIRayHit(hitDist.x, isMissDiff, isScreenDiff), GIRayHit(hitDist.y, isMissSpec, isScreenSpec), packed.y);
}

void GI_Store_RayHits(const int2 coord, const GIRayHits u)
{
    uint packed = packHalf2x16(float2(u.diff.dist, u.spec.dist));
    packed = u.diff.isMiss ? bitfieldInsert(packed, 0x7C00u, 0, 16) : packed;
    packed = u.spec.isMiss ? bitfieldInsert(packed, 0x7C00u, 16, 16) : packed;
    packed = bitfieldInsert(packed, u.diff.isScreen ? 0x1u : 0x0u, 15, 1);
    packed = bitfieldInsert(packed, u.spec.isScreen ? 0x1u : 0x0u, 31, 1);
    imageStore(pk_GI_RayHits, coord, uint4(packed, u.diffNormal, 0u, 0u));
}

float4 GI_Load_Voxel_UVW(const half3 uvw, float lvl) { return tex2DLod(pk_GI_VolumeRead, float3(uvw), lvl); }
float4 GI_Load_Voxel(const float3 worldpos, float lvl) { return tex2DLod(pk_GI_VolumeRead, GI_WorldToVoxelUVW(worldpos), lvl); }
float4 GI_Load_Voxel_Discrete(const float3 worldpos, float lvl) { return tex2DLod(pk_GI_VolumeRead, GI_WorldToVoxelUVWDiscrete(worldpos), lvl); }
void GI_Store_Voxel(float3 worldpos, float4 color) 
{ 
    int3 coord = GI_WorldToVoxelSpace(worldpos);
    imageStore(pk_GI_VolumeMaskWrite, coord, uint4(1u));
    imageStore(pk_GI_VolumeWrite, coord, color); 
}

//----------PREDICATES----------//
bool GI_Test_VX_History(const float2 uv) { return GI_Load_Diff(int2(uv * pk_ScreenSize.xy)).history < PK_GI_VX_MIN_HISTORY; }
bool GI_Test_VX_HasValue(float3 worldposition) { return imageLoad(pk_GI_VolumeMaskWrite, GI_WorldToVoxelSpace(worldposition)).x != 0; }
bool GI_Test_VX_Normal(float3 normal)
{
    normal = abs(normal);
    return normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.x] && normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.y];
}

//----------SAMPLING FUNCTIONS----------//
float3 GI_Sample_Diffuse(const float2 uv, const float3 N) { return SH_ToIrradiance(GI_Load_Diff(int2(uv * pk_ScreenSize.xy)).sh, N, pk_GI_ChromaBias); }
float3 GI_Sample_Specular(const float2 uv, const float3 N) { return GI_Load_Spec(int2(uv * pk_ScreenSize.xy)).radiance; }

void GI_Sample_Lighting(const float2 uv, const float3 N, const float3 V, const float R, inout float3 diffuse, inout float3 specular) 
{
    const int2 coord = int2(uv * pk_ScreenSize.xy);
    const GIDiff s_diff = GI_Load_Diff(coord);
    const GISpec s_spec = GI_Load_Spec(coord);
    diffuse = SH_ToIrradiance(s_diff.sh, N, pk_GI_ChromaBias) * pow(s_diff.ao, PK_GI_AO_DIFF_POWER);
    specular = s_spec.radiance * pow(s_spec.ao, PK_GI_AO_SPEC_POWER);
}

//----------VOXEL TRACING FUNCTIONS----------//
half4 GI_SphereTrace_Diffuse(float3 position)
{
    half4 C = half4(0.0hf.xxx, 1.0hf);
    half3 uvw = half3(GI_WorldToVoxelUVW(position));

    for (uint i = 0; i < PK_GI_VX_MIP_COUNT; ++i)
    {
        float level = i * 0.75f;
        half4 V = half4(GI_Load_Voxel_UVW(uvw, level));
        C.rgb += (1.0hf - C.a) * V.a * (V.rgb / max(1e-4hf, V.a));
        C.a = min(1.0hf, C.a + (1.0hf - C.a) * V.a);
        C.a *= clamp(1.0hf - V.a * (1.0hf + half(pow3(level)) * 0.075hf), 0.0hf, 1.0hf);
    }

    return C;
}

float4 GI_ConeTrace_Diffuse(const float3 O, const float3 N, const float dither) 
{
    const float angle = PK_PI / 3.0f;
    const float levelscale = 2.0f * tan(angle / 2.0f) / pk_GI_VoxelSize;
    const float correctionAngle = tan(angle / 8.0f);
    const float S = (1.0f + correctionAngle) / (1.0f - correctionAngle) * pk_GI_VoxelSize / 2.0f;
    
    float4 A = 0.0.xxxx;
    float3 T = cross(N, float3(0.0f, 1.0f, 0.0f));
    float3 B = cross(T, N);

    const float3 directions[6] =
    {
        N, 
        0.7071f * N + 0.7071f * T,
        0.7071f * N + 0.7071f * (0.309f * T + 0.951f * B),
        0.7071f * N + 0.7071f * (-0.809f * T + 0.588f * B),
        0.7071f * N - 0.7071f * (-0.809f * T - 0.588f * B),
        0.7071f * N - 0.7071f * (0.309f * T - 0.951f * B)
    };

    for (uint i = 0u; i < 6; ++i)
    {
        const float3 D = directions[i];
        
        float4 C = 0.0.xxxx;
        float AO = 1.0f;
        float DI = S;

        for (uint j = 0u; j < 11u; ++j)
        {
            float level = max(1.0f, log2(levelscale * DI));
            float4 V = GI_Load_Voxel(O + D * DI, level);
            C.rgb += (1.0f - C.a) * V.a * (V.rgb / max(1e-4f, V.a));
            C.a = min(1.0f, C.a + (1.0f - C.a) * V.a);
            DI += S * level;
            AO *= max(0.0f, 1.0f - V.a * (1.0f + level * 0.5f));
        }

        C.a = AO;
        A += C * max(0.0f, dot(N, D));
    }
 
    // Ground Occlusion
    A.a *= saturate(N.y + 1.0f);
    A /= 6.0f;

    return A;
}
