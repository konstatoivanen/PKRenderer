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
    float pk_GI_VoxelSize; 
    float pk_GI_ChromaBias; 
};

layout(r32ui, set = PK_SET_SHADER) uniform uimage2D pk_GI_RayHits;
layout(rg16f, set = PK_SET_SHADER) uniform image2DArray pk_GI_Meta_Write;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_GI_Meta_Read;

layout(rgba16f, set = PK_SET_SHADER) uniform image2DArray pk_GI_SHY_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2DArray pk_GI_CoCg_Write;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_GI_SHY_Read;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_GI_CoCg_Read;

layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_GI_VolumeMaskWrite;
layout(rgba16, set = PK_SET_SHADER) uniform image3D pk_GI_VolumeWrite;
PK_DECLARE_SET_SHADER uniform sampler3D pk_GI_VolumeRead;

#define PK_GI_MOMENTS_LVL 0
#define PK_GI_HISTVAR_LVL 1
#define PK_GI_DIFF_LVL 0
#define PK_GI_SPEC_LVL 1
#define PK_GI_VOXEL_MAX_MIP 7
#define PK_GI_RAY_MIN_DISTANCE 0.005f
#define PK_GI_RAY_MAX_DISTANCE 100.0f
#define PK_GI_MAX_HISTORY 256u
#define PK_GI_MIN_VXHISTORY 32.0

//----------UTILITIES----------//

uint2 MurmurHash21(uint src) 
{
    const uint M = 0x5bd1e995u;
    uint2 h = uint2(1190494759u, 2147483647u);
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

void GI_GetRayDirections(uint2 coord, uint offset, in float3 N, in float3 V, in float R, inout float3 dirDiff, inout float3 dirSpec)
{
    uint2 hash = MurmurHash21(offset / 64u);
    float3 v = GlobalNoiseBlue(coord.xy + hash, offset);
    float2 Xi = saturate(v.xy + ((v.z - 0.5f) / 256.0f));
    dirDiff = ImportanceSampleLambert(Xi, N);
    dirSpec = ImportanceSampleSmithGGX(Xi.yx, N, V, R);
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

float GI_SHToLuminance(SH sh, float3 dir) { return SHToLuminance(sh, dir, pk_GI_ChromaBias); }
bool GI_Test_VX_History(const float2 uv) { return texelFetch(pk_GI_Meta_Read, int3(uv * pk_ScreenSize.xy, PK_GI_HISTVAR_LVL), 0).x < PK_GI_MIN_VXHISTORY; }
bool GI_Test_VX_HasValue(float3 worldposition) { return imageLoad(pk_GI_VolumeMaskWrite, GI_WorldToVoxelSpace(worldposition)).x != 0; }
bool GI_Test_VX_Normal(float3 normal)
{
    normal = abs(normal);
    return normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.x] && normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.y];
}

//----------LOAD / STORE FUNCTIONS----------//
float2 GI_Load_Moments(int2 coord) { return texelFetch(pk_GI_Meta_Read, int3(coord, PK_GI_MOMENTS_LVL), 0).xy; }
float2 GI_Load_HistoryVariance(int2 coord) { return texelFetch(pk_GI_Meta_Read, int3(coord, PK_GI_HISTVAR_LVL), 0).xy; }

SH GI_Load_SH(const float2 uv, float lvl) { return SH(tex2D(pk_GI_SHY_Read, float3(uv, lvl)).rgba, tex2D(pk_GI_CoCg_Read, float3(uv, lvl)).rg); }
SH GI_Load_SH(const int2 coord, int lvl) { return SH(texelFetch(pk_GI_SHY_Read, int3(coord, lvl), 0).rgba, texelFetch(pk_GI_CoCg_Read, int3(coord, lvl), 0).rg); }
float3 GI_Load_Diffuse(const float2 uv, const float3 dir) { return SHToIrradiance(GI_Load_SH(uv, PK_GI_DIFF_LVL), dir, pk_GI_ChromaBias); }
float3 GI_Load_Specular(const float2 uv, const float3 dir) { return SHToIrradiance(GI_Load_SH(uv, PK_GI_SPEC_LVL), dir, pk_GI_ChromaBias); }

float4 GI_Load_VX(const float3 worldpos, float lvl) { return tex2DLod(pk_GI_VolumeRead, GI_WorldToVoxelUVW(worldpos), lvl); }
float4 GI_Load_VX_Discrete(const float3 worldpos, float lvl) { return tex2DLod(pk_GI_VolumeRead, GI_WorldToVoxelUVWDiscrete(worldpos), lvl); }

void GI_Store_Moments(int2 coord, float2 v) { imageStore(pk_GI_Meta_Write, int3(coord, PK_GI_MOMENTS_LVL), float4(v, 0.0f.xx)); }
void GI_Store_HistoryVariance(int2 coord, float2 v) { imageStore(pk_GI_Meta_Write, int3(coord, PK_GI_HISTVAR_LVL), float4(v, 0.0f.xx)); }

void GI_Store_SH(const int2 coord, const int level, const SH sh)
{
    imageStore(pk_GI_SHY_Write, int3(coord, level), sh.Y);
    imageStore(pk_GI_CoCg_Write, int3(coord, level), float4(sh.CoCg, 0.0f.xx));
}

void GI_Store_VX(float3 worldposition, float4 color) 
{ 
    int3 coord = GI_WorldToVoxelSpace(worldposition);
    imageStore(pk_GI_VolumeMaskWrite, coord, uint4(1u));
    imageStore(pk_GI_VolumeWrite, coord, color); 
}

//----------VOXEL CONE TRACING FUNCTIONS----------//
float4 GI_ConeTrace_Volumetric(float3 position)
{
    float4 color = float4(0.0.xxx, 1.0);

    for (uint i = 0; i < 4; ++i)
    {
        float level = i * 1.25f;
        float4 voxel = GI_Load_VX(position, level);
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
            float4 V = GI_Load_VX(O + D * DI, level);
            C.rgb += (1.0f - C.a) * V.a * (V.rgb / max(1e-4f, V.a));
            C.a = min(1.0f, C.a + (1.0f - C.a) * V.a);
            DI += S * level;

            AO *= max(0.0f, 1.0f - V.a * (1.0f + level * 0.5f));
        }

        C.a = AO;
        A += C * max(0.0f, dot(N, D));
    }
 
    float groundOcclusion = saturate(N.y + 1.0f);

    A.a *= groundOcclusion;
    A /= 6.0f;

    return A;
}
