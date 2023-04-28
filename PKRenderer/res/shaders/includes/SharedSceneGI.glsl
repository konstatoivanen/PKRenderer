#pragma once
#include Utilities.glsl
#include SampleDistribution.glsl
#include BlueNoise.glsl
#include SHL1.glsl

PK_DECLARE_CBUFFER(pk_SceneGI_Params, PK_SET_SHADER)
{
    float4 pk_SceneGI_ST;
    uint4 pk_SceneGI_Swizzle;
    int4 pk_SceneGI_Checkerboard_Offset;
    float pk_SceneGI_VoxelSize; 
    float pk_SceneGI_LuminanceGain; 
    float pk_SceneGI_ChrominanceGain; 
};

layout(r32ui, set = PK_SET_SHADER) uniform uimage2D pk_ScreenGI_Meta_Read;
layout(r32ui, set = PK_SET_SHADER) uniform uimage2D pk_ScreenGI_Meta_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2D pk_ScreenGI_Hits;
layout(rgba16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_SHY_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_CoCg_Write;

layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_SceneGI_VolumeMaskWrite;
layout(rgba16, set = PK_SET_SHADER) uniform image3D pk_SceneGI_VolumeWrite;

PK_DECLARE_SET_SHADER uniform sampler3D pk_SceneGI_VolumeRead;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_ScreenGI_SHY_Read;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_ScreenGI_CoCg_Read;

struct SceneGIMeta
{
    float2 moments;
    uint history;
};

#define PK_GI_DIFF_LVL 0
#define PK_GI_SPEC_LVL 1
#define PK_GI_VOXEL_MAX_MIP 7
#define PK_GI_VOXEL_SIZE pk_SceneGI_VoxelSize
#define PK_GI_VOXEL_LENGTH pk_SceneGI_VoxelSize * PK_SQRT2
#define PK_GI_CHECKERBOARD_OFFSET pk_SceneGI_Checkerboard_Offset.xy
#define PK_GI_RAY_MIN_DISTANCE 0.005f
#define PK_GI_RAY_MAX_DISTANCE 100.0f
#define PK_GI_MAX_HISTORY 255u

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

float2 GetSampleOffset(uint2 coord, uint offset)
{
    uint2 hash = MurmurHash21(offset / 64u);
    float3 v = GlobalNoiseBlue(coord.xy + hash, offset);
    return saturate(v.xy + ((v.z - 0.5f) / 256.0f));
}

float3 VoxelToWorldSpace(int3 coord) { return (float3(coord) * PK_GI_VOXEL_SIZE) + pk_SceneGI_ST.xyz + PK_GI_VOXEL_SIZE * 0.5f; }
int3 WorldToVoxelSpace(float3 worldposition) { return int3((worldposition - pk_SceneGI_ST.xyz) * pk_SceneGI_ST.www); }
float3 QuantizeWorldToVoxelSpace(float3 worldposition) { return VoxelToWorldSpace(WorldToVoxelSpace(worldposition)); }
float3 WorldToSampleSpace(float3 worldposition) { return ((worldposition - pk_SceneGI_ST.xyz) * pk_SceneGI_ST.www) / textureSize(pk_SceneGI_VolumeRead, 0).xyz;  }
float3 WorldToSampleSpaceDiscrete(float3 worldposition) { return (WorldToVoxelSpace(worldposition) + 0.5f.xxx) / textureSize(pk_SceneGI_VolumeRead, 0).xyz;  }
float3 WorldToVoxelClipSpace(float3 worldposition) { return WorldToSampleSpace(worldposition) * 2.0f - 1.0f; }

float4 WorldToVoxelNDCSpace(float3 worldposition) 
{ 
    float3 clippos = WorldToVoxelClipSpace(worldposition);
    return float4(clippos[pk_SceneGI_Swizzle.x], clippos[pk_SceneGI_Swizzle.y], clippos[pk_SceneGI_Swizzle.z] * 0.5f + 0.5f, 1);
}

//----------PREDICATES----------//
bool SceneGI_VoxelHasValue(float3 worldposition)
{
    int3 coord = WorldToVoxelSpace(worldposition);
    return imageLoad(pk_SceneGI_VolumeMaskWrite, coord).x != 0;
}

bool SceneGI_NormalReject(float3 normal)
{
    normal = abs(normal);
    return normal[pk_SceneGI_Swizzle.z] > normal[pk_SceneGI_Swizzle.x] && normal[pk_SceneGI_Swizzle.z] > normal[pk_SceneGI_Swizzle.y];
}

//----------META SAMPLE / STORE FUNCTIONS----------//
SceneGIMeta SceneGI_DecodeMeta(uint v)
{
    SceneGIMeta meta;
    meta.moments = DecodeE5GR9(v);
    meta.history = bitfieldExtract(v, 0, 9);
    return meta;
}

uint SceneGI_EncodeMeta(const SceneGIMeta m)
{
    return bitfieldInsert(EncodeE5GR9(m.moments), min(PK_GI_MAX_HISTORY, m.history), 0, 9);
}

uint SampleGI_MetaEnc(int2 coord) { return imageLoad(pk_ScreenGI_Meta_Read, coord).x; }
void StoreGI_MetaEnc(int2 coord, const uint meta) { imageStore(pk_ScreenGI_Meta_Write, coord, uint4(meta)); }
SceneGIMeta SampleGI_Meta(int2 coord) { return SceneGI_DecodeMeta(SampleGI_MetaEnc(coord)); }
void StoreGI_Meta(int2 coord, const SceneGIMeta meta) { StoreGI_MetaEnc(coord, SceneGI_EncodeMeta(meta)); }

//----------VOXEL SAMPLE / STORE FUNCTIONS----------//
float4 SampleGI_WS(float3 worldposition, float level)
{
    return tex2DLod(pk_SceneGI_VolumeRead, WorldToSampleSpace(worldposition), level);
}

float4 SampleGI_WS_Discrete(float3 worldposition, float level)
{
    return tex2DLod(pk_SceneGI_VolumeRead, WorldToSampleSpaceDiscrete(worldposition), level);
}

void StoreGI_WS(float3 worldposition, float4 color) 
{ 
    int3 coord = WorldToVoxelSpace(worldposition);
    imageStore(pk_SceneGI_VolumeMaskWrite, coord, uint4(1u));
    imageStore(pk_SceneGI_VolumeWrite, coord, color); 
}

//----------SH SAMPLE / STORE FUNCTIONS----------//
SH SampleGI_SH(const float2 uv, float level)
{
    return SH
    (
        tex2D(pk_ScreenGI_SHY_Read, float3(uv, level)).rgba, 
        tex2D(pk_ScreenGI_CoCg_Read, float3(uv, level)).rg
    );
}

SH SampleGI_SH(const int2 coord, int level)
{
    return SH
    (
        texelFetch(pk_ScreenGI_SHY_Read, int3(coord, level), 0).rgba, 
        texelFetch(pk_ScreenGI_CoCg_Read, int3(coord, level), 0).rg
    );
}

void StoreGI_SH(const int2 coord, const int level, const SH sh)
{
    imageStore(pk_ScreenGI_SHY_Write, int3(coord, level), sh.Y);
    imageStore(pk_ScreenGI_CoCg_Write, int3(coord, level), float4(sh.CoCg, 0.0f.xx));
}

float3 SampleGI_VS_Diffuse(const float2 uv, const float3 direction)
{
    SH diffSH = SampleGI_SH(uv, PK_GI_DIFF_LVL);
    diffSH.Y *= pk_SceneGI_LuminanceGain;
    diffSH.CoCg *= pk_SceneGI_ChrominanceGain;
    return SHToIrradiance(diffSH, direction);
}

void SampleGI_VS(inout float3 diffuse, inout float3 specular, const float2 uv, const float3 N, const float3 V, const float R)
{
    SH diffSH = SampleGI_SH(uv, PK_GI_DIFF_LVL);
    diffSH.Y *= pk_SceneGI_LuminanceGain;
    diffSH.CoCg *= pk_SceneGI_ChrominanceGain;
    diffuse = SHToIrradiance(diffSH, N);
    
    SH specSH = SampleGI_SH(uv, PK_GI_SPEC_LVL);
    specSH.Y *= pk_SceneGI_LuminanceGain;
    specSH.CoCg *= pk_SceneGI_ChrominanceGain;
    specular = SHToIrradiance(specSH, normalize(reflect(V, N)));
}

//----------VOXEL CONE TRACING FUNCTIONS----------//
float4 SampleGI_ConeTraceVolumetric(float3 position)
{
    float4 color = float4(0.0.xxx, 1.0);

    for (uint i = 0; i < 4; ++i)
    {
        float level = i * 1.25f;

        float4 voxel = SampleGI_WS(position, level);

        color.rgb += voxel.rgb * (1.0 + level) * pow2(color.a) * i;
        color.a *= saturate(1.0 - voxel.a * (1.0 + pow3(level) * 0.075));
    }

    return color;
}

float4 SampleGI_ConeTraceDiffuse(const float3 O, const float3 N, const float dither) 
{
    const float angle = PK_PI / 3.0f;
    const float levelscale = 2.0f * tan(angle / 2.0f) / PK_GI_VOXEL_SIZE;
    const float correctionAngle = tan(angle / 8.0f);
    const float S = (1.0f + correctionAngle) / (1.0f - correctionAngle) * PK_GI_VOXEL_SIZE / 2.0f;
    
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
            float4 V = SampleGI_WS(O + D * DI, level);
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
