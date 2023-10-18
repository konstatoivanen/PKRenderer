#pragma once
#include SceneGI.glsl

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
float3 GI_WorldToVoxelUVW(float3 worldpos) { return ((worldpos - pk_GI_VolumeST.xyz) * pk_GI_VolumeST.www) / textureSize(pk_GI_VolumeRead, 0).xyz;  }
float3 GI_WorldToVoxelUVWDiscrete(float3 worldpos) { return (GI_WorldToVoxelSpace(worldpos) + 0.5f.xxx) / textureSize(pk_GI_VolumeRead, 0).xyz;  }
float3 GI_WorldToVoxelClipSpace(float3 worldpos) { return GI_WorldToVoxelUVW(worldpos) * 2.0f - 1.0f; }
float4 GI_WorldToVoxelNDCSpace(float3 worldpos) 
{ 
    float3 clippos = GI_WorldToVoxelClipSpace(worldpos);
    return float4(clippos[pk_GI_VolumeSwizzle.x], clippos[pk_GI_VolumeSwizzle.y], clippos[pk_GI_VolumeSwizzle.z] * 0.5f + 0.5f, 1);
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

half4 GI_ConeTrace_Diffuse(const float3 O, const float3 N) 
{
    const float angle = PK_PI / 3.0f;
    const float levelscale = 2.0f * tan(angle / 2.0f) / pk_GI_VoxelSize;
    const float correctionAngle = tan(angle / 8.0f);
    const float S = (1.0f + correctionAngle) / (1.0f - correctionAngle) * pk_GI_VoxelSize / 2.0f;

    const half3 directions[6] =
    {
        half3( 0.0hf,        0.0hf,       1.0hf), 
        half3( 0.7071hf,     0.0hf,       0.7071hf),
        half3( 0.2184939hf,  0.6724521hf, 0.7071hf),
        half3(-0.5720439hf,  0.4157748hf, 0.7071hf),
        half3( 0.5720439hf,  0.4157748hf, 0.7071hf),
        half3(-0.2184939hf,  0.6724521hf, 0.7071hf)
    };
    
    float3x3 TBN = make_TBN(N);
    
    half4 A = 0.0hf.xxxx;

    [[unroll]]
    for (uint i = 0u; i < 6; ++i)
    {
        const float3 D = mul(TBN, directions[i]);
        
        half4 C = 0.0hf.xxxx;
        half AO = 1.0hf;
        half DI = half(S);

        [[unroll]]
        for (uint j = 0u; j < 11u; ++j)
        {
            half level = max(1.0hf, log2(half(levelscale) * DI));
            half4 V = half4(GI_Load_Voxel(O + D * DI, level));
            C += (1.0hf - C.a) * V;
            DI += half(S) * level;
            AO *= max(0.0hf, 1.0hf - V.a * (1.0hf + level * 0.5hf));
        }

        C.a = AO;
        A += C * half(dot(N, D));
    }
 
    // Ground Occlusion
    A.a *= clamp(half(N.y) + 1.0hf, 0.0hf, 1.0hf);

    return A / 6.0hf;
}
