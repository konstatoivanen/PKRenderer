#pragma once
#include "SceneGI.glsl"

uniform uimage3D pk_GI_VX_Mask;
uniform image3D pk_GI_VX_RadianceWrite;
uniform texture3D pk_GI_VX_RadianceRead;

#define PK_GI_VX_MIP_COUNT 7
#define PK_GI_VX_MIN_HISTORY 4.0f
#define PK_GI_VX_CONE_SIZE 0.25f

//----------UTILITIES----------//
float3 GI_VX_CoordToWorld(int3 coord) { return (coord + 0.5f.xxx) / pk_GI_VX_ST.www + pk_GI_VX_ST.xyz; }
int3   GI_VX_WorldToCoord(float3 world_pos) { return int3((world_pos - pk_GI_VX_ST.xyz) * pk_GI_VX_ST.www); }
float3 GI_VX_WorldToUvw(float3 world_pos) { return (world_pos - pk_GI_VX_ST.xyz) * pk_GI_VX_ST.www * pk_GI_VX_TexelSize;  }

float4 GI_VX_WorldToNdc(float3 world_pos) 
{ 
    const float3 clippos = GI_VX_WorldToUvw(world_pos) * 2.0f - 1.0f;
    return float4(clippos[pk_GI_VX_Swizzle.x], clippos[pk_GI_VX_Swizzle.y], clippos[pk_GI_VX_Swizzle.z] * 0.5f + 0.5f, 1);
}

float3 GI_VX_FragToWorld(float3 fcoord)
{
    fcoord.z /= pk_GI_VX_TexelSize[pk_GI_VX_Swizzle.z];
    float3 coord = fcoord;
    coord[pk_GI_VX_Swizzle.x] = fcoord.x;
    coord[pk_GI_VX_Swizzle.y] = fcoord.y;
    coord[pk_GI_VX_Swizzle.z] = fcoord.z;
    return coord / pk_GI_VX_ST.www + pk_GI_VX_ST.xyz;
}

//----------LOAD/STORE FUNCTIONS----------//
float4 GI_VX_Load_Uvw(const half3 uvw, float lvl) { return textureLod(sampler3D(pk_GI_VX_RadianceRead, pk_SamplerTrilinearBorder), float3(uvw), lvl); }
float4 GI_VX_Load(const float3 world_pos, float lvl) { return textureLod(sampler3D(pk_GI_VX_RadianceRead, pk_SamplerTrilinearBorder), GI_VX_WorldToUvw(world_pos), lvl); }
float4 GI_VX_Load_Point(const float3 world_pos, float lvl) { return textureLod(sampler3D(pk_GI_VX_RadianceRead, pk_SamplerPointClamped), GI_VX_WorldToUvw(world_pos), lvl); }

void GI_VX_Store(float3 world_pos, float4 color) 
{ 
    int3 coord = GI_VX_WorldToCoord(world_pos);
    imageStore(pk_GI_VX_Mask, coord, uint4(1u));
    imageStore(pk_GI_VX_RadianceWrite, coord, color); 
}

//----------PREDICATES----------//
bool GI_VX_Test_HasValue(float3 world_pos) 
{ 
    return imageLoad(pk_GI_VX_Mask, GI_VX_WorldToCoord(world_pos)).x != 0; 
}

bool GI_VX_Test_Normal(float3 normal)
{
    normal = abs(normal);
    return normal[pk_GI_VX_Swizzle.z] > normal[pk_GI_VX_Swizzle.x] && normal[pk_GI_VX_Swizzle.z] > normal[pk_GI_VX_Swizzle.y];
}

//----------VOXEL TRACING FUNCTIONS----------//
half4 GI_VX_SphereTrace_Diffuse(float3 position)
{
    half4 C = 0.0hf.xxxx;
    half3 uvw = half3(GI_VX_WorldToUvw(position));
    half AO = 1.0hf;

    for (uint i = 0; i < PK_GI_VX_MIP_COUNT; ++i)
    {
        float level = i * 0.75f + 0.5f;
        half4 V = half4(GI_VX_Load_Uvw(uvw, level));
        C += (1.0hf - C.a) * V;
        AO *= max(0.0hf, 1.0hf - V.a * (1.0hf + half(level) * 0.5hf));
    }

    return half4(C.rgb, AO);
}

half4 GI_VX_ConeTrace_Diffuse(const float3 origin, const float3 normal) 
{
    half3x3 basis = half3x3(CreateTBN(normal));
    basis[0] *= half3(pk_GI_VX_ST.www * pk_GI_VX_TexelSize);
    basis[1] *= half3(pk_GI_VX_ST.www * pk_GI_VX_TexelSize);
    basis[2] *= half3(pk_GI_VX_ST.www * pk_GI_VX_TexelSize);

    const half3 uvw_start = half3(GI_VX_WorldToUvw(origin));

    half4 accumulation = 0.0hf.xxxx;

    [[loop]]
    for (uint i = 0u; i < 6; ++i)
    {
        const half isLast = step(5.0hf, half(i));
        const half f = half(PK_TWO_PI) * half(i) / 5.0hf;
        const half3 direction = basis * normalize(half3(sin(f) * isLast, cos(f) * isLast, 1.0hf));
        
        half4 value = 0.0hf.xxxx;
        half occlusion = 1.0hf;
        half t = half(pk_GI_VX_StepSize) * 2.0hf;

        [[unroll]]
        for (uint j = 0u; j < 11u; ++j)
        {
            half level = max(1.0hf, log2(half(pk_GI_VX_LevelScale) * t));
            half4 voxel = half4(GI_VX_Load_Uvw(uvw_start + direction * t, level));
            t += half(pk_GI_VX_StepSize) * level;
            value += (1.0hf - value.a) * voxel;
            occlusion *= max(0.0hf, 1.0hf - voxel.a * (1.0hf + level * 0.5hf));
        }

        value.a = occlusion;
        accumulation += value;
    }
 
    // Ground Occlusion
    accumulation.a *= clamp(half(normal.y) + 1.0hf, 0.0hf, 1.0hf);

    return accumulation / 6.0hf;
}
