#pragma once
#include "SceneGI.glsl"

layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_GI_VolumeMaskWrite;
layout(rgba16f, set = PK_SET_SHADER) uniform image3D pk_GI_VolumeWrite;
PK_DECLARE_SET_SHADER uniform sampler3D pk_GI_VolumeRead;

#define PK_GI_VX_MIP_COUNT 7
#define PK_GI_VX_MIN_HISTORY 4.0f
#define PK_GI_VX_CONE_SIZE 0.25f

//----------UTILITIES----------//
float3 GI_VoxelToWorldSpace(int3 coord) { return coord * pk_GI_VoxelSize + pk_GI_VolumeST.xyz + pk_GI_VoxelSize * 0.5f; }
int3   GI_WorldToVoxelSpace(float3 worldpos) { return int3((worldpos - pk_GI_VolumeST.xyz) * pk_GI_VolumeST.www); }
float3 GI_QuantizeWorldToVoxelSpace(float3 worldpos) { return GI_VoxelToWorldSpace(GI_WorldToVoxelSpace(worldpos)); }
float3 GI_GetVoxelTexelScale() { return pk_GI_VolumeST.www / textureSize(pk_GI_VolumeRead, 0).xyz; }
float3 GI_WorldToVoxelUVW(float3 worldpos) { return ((worldpos - pk_GI_VolumeST.xyz) * pk_GI_VolumeST.www) / textureSize(pk_GI_VolumeRead, 0).xyz;  }
float3 GI_WorldToVoxelUVWDiscrete(float3 worldpos) { return (GI_WorldToVoxelSpace(worldpos) + 0.5f.xxx) / textureSize(pk_GI_VolumeRead, 0).xyz;  }
float3 GI_WorldToVoxelClipSpace(float3 worldpos) { return GI_WorldToVoxelUVW(worldpos) * 2.0f - 1.0f; }
float4 GI_WorldToVoxelNDCSpace(float3 worldpos) 
{ 
    float3 clippos = GI_WorldToVoxelClipSpace(worldpos);
    return float4(clippos[pk_GI_VolumeSwizzle.x], clippos[pk_GI_VolumeSwizzle.y], clippos[pk_GI_VolumeSwizzle.z] * 0.5f + 0.5f, 1);
}
float3 GI_FragVoxelToWorldSpace(float3 fragCoord)
{
    fragCoord.z *= textureSize(pk_GI_VolumeRead, 0)[pk_GI_VolumeSwizzle.z];
    float3 coord = fragCoord;
    coord[pk_GI_VolumeSwizzle.x] = fragCoord.x;
    coord[pk_GI_VolumeSwizzle.y] = fragCoord.y;
    coord[pk_GI_VolumeSwizzle.z] = fragCoord.z;
    return coord * pk_GI_VoxelSize + pk_GI_VolumeST.xyz;
}

//----------LOAD/STORE FUNCTIONS----------//
float4 GI_Load_Voxel_UVW(const half3 uvw, float lvl) { return textureLod(pk_GI_VolumeRead, float3(uvw), lvl); }
float4 GI_Load_Voxel(const float3 worldpos, float lvl) { return textureLod(pk_GI_VolumeRead, GI_WorldToVoxelUVW(worldpos), lvl); }
float4 GI_Load_Voxel_Discrete(const float3 worldpos, float lvl) { return textureLod(pk_GI_VolumeRead, GI_WorldToVoxelUVWDiscrete(worldpos), lvl); }
void GI_Store_Voxel(float3 worldpos, float4 color) 
{ 
    int3 coord = GI_WorldToVoxelSpace(worldpos);
    imageStore(pk_GI_VolumeMaskWrite, coord, uint4(1u));
    imageStore(pk_GI_VolumeWrite, coord, color); 
}

//----------PREDICATES----------//
bool GI_Test_VX_HasValue(float3 worldposition) { return imageLoad(pk_GI_VolumeMaskWrite, GI_WorldToVoxelSpace(worldposition)).x != 0; }
bool GI_Test_VX_Normal(float3 normal)
{
    normal = abs(normal);
    return normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.x] && normal[pk_GI_VolumeSwizzle.z] > normal[pk_GI_VolumeSwizzle.y];
}

//----------VOXEL TRACING FUNCTIONS----------//
half4 GI_SphereTrace_Diffuse(float3 position)
{
    half4 C = 0.0hf.xxxx;
    half3 uvw = half3(GI_WorldToVoxelUVW(position));
    half AO = 1.0hf;

    for (uint i = 0; i < PK_GI_VX_MIP_COUNT; ++i)
    {
        float level = i * 0.75f + 0.5f;
        half4 V = half4(GI_Load_Voxel_UVW(uvw, level));
        C += (1.0hf - C.a) * V;
        AO *= max(0.0hf, 1.0hf - V.a * (1.0hf + half(level) * 0.5hf));
    }

    return half4(C.rgb, AO);
}

half4 GI_ConeTrace_Diffuse(const float3 origin, const float3 normal) 
{
    const float3x3 basis = make_TBN(normal);

    half4 accumulation = 0.0hf.xxxx;

    [[loop]]
    for (uint i = 0u; i < 6; ++i)
    {
        const float isLast = step(5.0f, i);
        const float f = (i / 5.0f) * PK_TWO_PI;
        const float3 direction = basis * normalize(float3(sin(f) * isLast, cos(f) * isLast, 1.0f));
        
        half4 value = 0.0hf.xxxx;
        half occlusion = 1.0hf;
        half t = half(pk_GI_VoxelStepSize) * 2.0hf; // Skip first texel of mip 1

        [[unroll]]
        for (uint j = 0u; j < 11u; ++j)
        {
            half level = max(1.0hf, log2(half(pk_GI_VoxelLevelScale) * t));
            half4 voxel = half4(GI_Load_Voxel(origin + direction * t, level));

            value += (1.0hf - value.a) * voxel;
            occlusion *= max(0.0hf, 1.0hf - voxel.a * (1.0hf + level * 0.5hf));
            t += half(pk_GI_VoxelStepSize) * level;
        }

        value.a = occlusion;
        // cosine distribution. no need to multiply half(lerp(PK_INV_SQRT2, 1.0f, isLast));
        accumulation += value;
    }
 
    // Ground Occlusion
    accumulation.a *= clamp(half(normal.y) + 1.0hf, 0.0hf, 1.0hf);

    return accumulation / 6.0hf;
}
