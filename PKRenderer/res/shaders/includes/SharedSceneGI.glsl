#pragma once
#include Utilities.glsl
#include SampleDistribution.glsl
#include BlueNoise.glsl
#include SHL1.glsl

PK_DECLARE_CBUFFER(pk_GI_Parameters, PK_SET_SHADER)
{
    float4 pk_GI_VolumeST;
    uint4 pk_GI_VolumeSwizzle;
    int4 pk_GI_Checkerboard_Offset;
    uint2 pk_GI_RayDither;
    float pk_GI_VoxelSize; 
    float pk_GI_ChromaBias; 
};

layout(r32ui, set = PK_SET_SHADER) uniform uimage2D pk_GI_RayHits;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly uimage2DArray pk_GI_ScreenDataWrite;
layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_GI_VolumeMaskWrite;
layout(rgba16f, set = PK_SET_SHADER) uniform image3D pk_GI_VolumeWrite;
PK_DECLARE_SET_SHADER uniform usampler2DArray pk_GI_ScreenDataMips;
PK_DECLARE_SET_SHADER uniform usampler2DArray pk_GI_ScreenDataRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_GI_VolumeRead;

#define PK_GI_LVL_DIFF0 0
#define PK_GI_LVL_DIFF1 1
#define PK_GI_LVL_SPEC 2
#define PK_GI_LVL_META 3
#define PK_GI_VOXEL_MAX_MIP 7
#define PK_GI_RAY_MIN_DISTANCE 0.005f
#define PK_GI_RAY_MAX_DISTANCE 100.0f
#define PK_GI_AO_DIFF_MAX_DISTANCE 6.0f
#define PK_GI_AO_SPEC_MAX_DISTANCE 1.0f
#define PK_GI_AO_DIFF_POWER 0.25f
#define PK_GI_AO_SPEC_POWER 0.4f
#define PK_GI_MIN_ROUGH_SPEC 0.4f
#define PK_GI_MAX_ROUGH_SPEC 0.5f
#define PK_GI_APPROX_ROUGH_SPEC 1
#define PK_GI_MAX_HISTORY 256u
#define PK_GI_MIN_VXHISTORY 32.0
#define PK_GI_SCREEN_MAX_MIP 4
#define PK_GI_DATA_LOAD_MIP(c, l, m) texelFetch(pk_GI_ScreenDataMips, int3(c, l), m).xy
#define PK_GI_DATA_LOAD(c, l) texelFetch(pk_GI_ScreenDataRead, int3(c, l), 0).xy
#define PK_GI_DATA_STORE(c, l, d) imageStore(pk_GI_ScreenDataWrite, int3(c, l), uint4(d, 0,0))

//----------STRUCTS----------//
struct GISampleDiff
{
    SH sh;
    float ao;
    float depth;
};

struct GISampleSpec
{
    float3 radiance;
    float ao;
    float depth;
};

struct GISampleMeta
{
    float historyDiff;
    float historySpec;
    float2 moments;
};

struct GISampleFull
{
    GISampleDiff diff;
    GISampleSpec spec;
    GISampleMeta meta;
};

struct GIRayDirections
{
    float3 diff;
    float3 spec;
};

struct GIRayHits
{
    float distDiff;
    float distSpec;
    bool isMissDiff;
    bool isMissSpec;
};

#define pk_Zero_GISampleDiff GISampleDiff(pk_ZeroSH, 0.0f, 0.0f)
#define pk_Zero_GISampleSpec GISampleSpec(0.0f.xxx, 0.0f, 0.0f)
#define pk_Zero_GISampleMeta GISampleMeta(0.0f, 0.0f, 0.0f.xx)
#define pk_Zero_GISampleFull GISampleFull(pk_Zero_GISampleDiff, pk_Zero_GISampleSpec, pk_Zero_GISampleMeta)

//----------UTILITIES----------//
GIRayDirections GI_GetRayDirections(uint2 coord, in float3 N, in float3 V, in float R)
{
    const float3 v = GlobalNoiseBlue(coord.xy + pk_GI_RayDither, pk_FrameIndex.y);
    const float2 Xi = saturate(v.xy + ((v.z - 0.5f) / 256.0f));
    return GIRayDirections(ImportanceSampleLambert(Xi, N), ImportanceSampleSmithGGX(Xi.yx, N, V, R));
}

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
uint4 GI_Pack_SampleDiff(const GISampleDiff u) { return uint4(packHalf4x16(u.sh.Y), packHalf4x16(float4(u.sh.CoCg, u.ao, u.depth))); }
uint2 GI_Pack_SampleSpec(const GISampleSpec u) { return uint2(EncodeE5BGR9(u.radiance), packHalf2x16(float2(u.ao, u.depth))); }
uint2 GI_Pack_SampleMeta(const GISampleMeta u) { return packHalf4x16(float4(u.historyDiff, u.historySpec, u.moments)); }

GISampleDiff GI_Unpack_SampleDiff(const uint4 p) 
{
    const float4 v0 = unpackHalf4x16(p.xy);
    const float4 v1 = unpackHalf4x16(p.zw);
    return GISampleDiff(SH(v0, v1.xy), v1.z, v1.w);
}

GISampleSpec GI_Unpack_SampleSpec(const uint2 p) 
{ 
    const float2 v = unpackHalf2x16(p.y);
    return GISampleSpec(DecodeE5BGR9(p.x), v.x, v.y);
}

GISampleMeta GI_Unpack_SampleMeta(const uint2 p) 
{ 
    const float4 v0 = unpackHalf4x16(p.xy);
    return GISampleMeta(v0.x, v0.y, v0.zw);
}

//----------LOAD FUNCTIONS----------//
uint4 GI_Load_Packed_Mip_SampleDiff(const int2 coord, const int mip) { return uint4(PK_GI_DATA_LOAD_MIP(coord, PK_GI_LVL_DIFF0, mip), PK_GI_DATA_LOAD_MIP(coord, PK_GI_LVL_DIFF1, mip)); }
uint2 GI_Load_Packed_Mip_SampleSpec(const int2 coord, const int mip) { return PK_GI_DATA_LOAD_MIP(coord, PK_GI_LVL_SPEC, mip); }

uint4 GI_Load_Packed_SampleDiff(const int2 coord) { return uint4(PK_GI_DATA_LOAD(coord, PK_GI_LVL_DIFF0), PK_GI_DATA_LOAD(coord, PK_GI_LVL_DIFF1)); }
uint2 GI_Load_Packed_SampleSpec(const int2 coord) { return PK_GI_DATA_LOAD(coord, PK_GI_LVL_SPEC); }
uint2 GI_Load_Packed_SampleMeta(const int2 coord) { return PK_GI_DATA_LOAD(coord, PK_GI_LVL_META); }

GISampleDiff GI_Load_SampleDiff(const int2 coord) { return GI_Unpack_SampleDiff(GI_Load_Packed_SampleDiff(coord)); }
GISampleSpec GI_Load_SampleSpec(const int2 coord) { return GI_Unpack_SampleSpec(GI_Load_Packed_SampleSpec(coord)); }
GISampleMeta GI_Load_SampleMeta(const int2 coord) { return GI_Unpack_SampleMeta(GI_Load_Packed_SampleMeta(coord)); }
GISampleFull GI_Load_SampleFull(const int2 coord) { return GISampleFull(GI_Load_SampleDiff(coord), GI_Load_SampleSpec(coord), GI_Load_SampleMeta(coord)); }

GIRayHits GI_Load_RayHits(const int2 coord)
{
    const uint packed = imageLoad(pk_GI_RayHits, coord).x;
    const bool isMissDiff = bitfieldExtract(packed, 0, 16) == 0x7C00u;
    const bool isMissSpec = bitfieldExtract(packed, 16, 16)== 0x7C00u;
    const float2 hitDist = unpackHalf2x16(packed);
    return GIRayHits(hitDist.x, hitDist.y, isMissDiff, isMissSpec);
}

float4 GI_Load_Voxel(const float3 worldpos, float lvl) { return tex2DLod(pk_GI_VolumeRead, GI_WorldToVoxelUVW(worldpos), lvl); }
float4 GI_Load_Voxel_Discrete(const float3 worldpos, float lvl) { return tex2DLod(pk_GI_VolumeRead, GI_WorldToVoxelUVWDiscrete(worldpos), lvl); }

//----------STORE FUNCTIONS----------//
void GI_Store_Packed_SampleDiff(const int2 coord, const uint4 p) { PK_GI_DATA_STORE(coord, PK_GI_LVL_DIFF0, p.xy); PK_GI_DATA_STORE(coord, PK_GI_LVL_DIFF1, p.zw); }
void GI_Store_Packed_SampleSpec(const int2 coord, const uint2 p) { PK_GI_DATA_STORE(coord, PK_GI_LVL_SPEC, p); }
void GI_Store_Packed_SampleMeta(const int2 coord, const uint2 p) { PK_GI_DATA_STORE(coord, PK_GI_LVL_META, p); }
void GI_Store_Packed_SampleFull(const int2 coord, const uint4 u0, const uint2 u1, const uint2 u2) 
{ 
    GI_Store_Packed_SampleDiff(coord, u0); 
    GI_Store_Packed_SampleSpec(coord, u1); 
    GI_Store_Packed_SampleMeta(coord, u2); 
}

void GI_Store_SampleDiff(const int2 coord, const GISampleDiff u) { GI_Store_Packed_SampleDiff(coord, GI_Pack_SampleDiff(u)); }
void GI_Store_SampleSpec(const int2 coord, const GISampleSpec u) { GI_Store_Packed_SampleSpec(coord, GI_Pack_SampleSpec(u)); }
void GI_Store_SampleMeta(const int2 coord, const GISampleMeta u) { GI_Store_Packed_SampleMeta(coord, GI_Pack_SampleMeta(u)); }
void GI_Store_SampleFull(const int2 coord, const GISampleFull u) { GI_Store_Packed_SampleFull(coord, GI_Pack_SampleDiff(u.diff), GI_Pack_SampleSpec(u.spec), GI_Pack_SampleMeta(u.meta)); }

void GI_Store_RayHits(const int2 coord, const GIRayHits u)
{
    uint packed = packHalf2x16(float2(u.distDiff, u.distSpec));
    packed = u.isMissDiff ? bitfieldInsert(packed, 0x7C00u, 0, 16) : packed;
    packed = u.isMissSpec ? bitfieldInsert(packed, 0x7C00u, 16, 16) : packed;
    imageStore(pk_GI_RayHits, coord, uint4(packed));
}

void GI_Store_Voxel(float3 worldpos, float4 color) 
{ 
    int3 coord = GI_WorldToVoxelSpace(worldpos);
    imageStore(pk_GI_VolumeMaskWrite, coord, uint4(1u));
    imageStore(pk_GI_VolumeWrite, coord, color); 
}

//----------PREDICATES----------//
bool GI_Test_VX_History(const float2 uv) { return GI_Load_SampleMeta(int2(uv * pk_ScreenSize.xy)).historyDiff < PK_GI_MIN_VXHISTORY; }
bool GI_Test_VX_HasValue(float3 worldposition) { return imageLoad(pk_GI_VolumeMaskWrite, GI_WorldToVoxelSpace(worldposition)).x != 0; }
bool GI_Test_VX_Normal(float3 normal)
{
    normal = abs(normal);
    return normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.x] && normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.y];
}

//----------SAMPLING FUNCTIONS----------//
float GI_AOPower(float ao) { return pow(ao, 0.4f); }

float3 GI_Sample_Diffuse(const float2 uv, const float3 N)
{
    const GISampleDiff s_diff = GI_Load_SampleDiff(int2(uv * pk_ScreenSize.xy));
    return SHToIrradiance(s_diff.sh, N, pk_GI_ChromaBias);
}

float3 GI_Sample_Specular(const float2 uv, const float3 N)
{
    const GISampleSpec s_spec = GI_Load_SampleSpec(int2(uv * pk_ScreenSize.xy));
    return s_spec.radiance;
}

void GI_Sample_Lighting(const float2 uv, const float3 N, const float3 V, const float R, inout float3 diffuse, inout float3 specular) 
{
    //@ Todo get rough specular from diffuse dominant dir.
    const int2 coord = int2(uv * pk_ScreenSize.xy);
    const GISampleDiff s_diff = GI_Load_SampleDiff(coord);
    const GISampleSpec s_spec = GI_Load_SampleSpec(coord);
    diffuse = SHToIrradiance(s_diff.sh, N, pk_GI_ChromaBias);
    specular = s_spec.radiance;
    diffuse *= pow(s_diff.ao, PK_GI_AO_DIFF_POWER);
    specular *= pow(s_spec.ao, PK_GI_AO_SPEC_POWER);
}

//----------VOXEL CONE TRACING FUNCTIONS----------//
float4 GI_ConeTrace_Volumetric(float3 position)
{
    float4 color = float4(0.0.xxx, 1.0);

    #pragma unroll 4
    for (uint i = 0; i < 4; ++i)
    {
        float level = i * 1.25f;
        float4 voxel = GI_Load_Voxel(position, level);
        color.rgb += voxel.rgb * (1.0 + level) * pow2(color.a) * i;
        color.a *= saturate(1.0 - voxel.a * (1.0 + pow3(level) * 0.075));
    }

    return color;
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
