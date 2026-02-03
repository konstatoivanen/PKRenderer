#pragma once

#include "Utilities.glsl"
#include "SHL1.glsl"

#define PK_SCENE_ENV_IBL_MAX_MIP 4
#define PK_SCENE_ENV_ISL_MAX_MIP 7
#define PK_SCENE_ENV_MIN_SIZE 32

uniform sampler2D pk_SceneEnv;
uniform sampler2D pk_SceneEnv_ISL;
PK_DECLARE_BUFFER(float4, pk_SceneEnv_SH);

float3 SceneEnv_Sample_IBL(float2 uv, float roughness) 
{ 
    return textureLod(pk_SceneEnv, uv, roughness * PK_SCENE_ENV_IBL_MAX_MIP).rgb; 
}

float3 SceneEnv_Sample_ISL(float2 uv, float level)
{
    return textureLod(pk_SceneEnv_ISL, uv, level).rgb; 
}

float3 SceneEnv_Sample_ISL_Dual(float2 uv, float directionality)
{
    const float3 directional = textureLod(pk_SceneEnv_ISL, uv, 0.0f).rgb;
    const float3 ambient = textureLod(pk_SceneEnv_ISL, uv, PK_SCENE_ENV_ISL_MAX_MIP).rgb;
    return lerp(ambient, directional, directionality);
}

float3 SceneEnv_Sample_SH(float4 basis)
{
    const float R = max(0.0f, dot(PK_BUFFER_DATA(pk_SceneEnv_SH, 0), basis));
    const float G = max(0.0f, dot(PK_BUFFER_DATA(pk_SceneEnv_SH, 1), basis));
    const float B = max(0.0f, dot(PK_BUFFER_DATA(pk_SceneEnv_SH, 2), basis));
    return float3(R, G, B);
}

float3 SceneEnv_Sample_SH_PeakDirection()
{
    float3 direction = 0.0f.xxx;
    direction += PK_BUFFER_DATA(pk_SceneEnv_SH, 0).yzw * PK_LUMA_BT709.r;
    direction += PK_BUFFER_DATA(pk_SceneEnv_SH, 1).yzw * PK_LUMA_BT709.g;
    direction += PK_BUFFER_DATA(pk_SceneEnv_SH, 2).yzw * PK_LUMA_BT709.b;
    return direction / (length(direction) + 1e-6f); 
}

float3 SceneEnv_Sample_SH_Color()
{
    const float R = PK_BUFFER_DATA(pk_SceneEnv_SH, 0).x;
    const float G = PK_BUFFER_DATA(pk_SceneEnv_SH, 1).x;
    const float B = PK_BUFFER_DATA(pk_SceneEnv_SH, 2).x;
    return float3(R, G, B) / PK_L1BASIS.xxx;
}

float3 SceneEnv_Sample_SH_Diffuse(float3 direction)  
{ 
    const float R = SH_EvaluateDiffuse(PK_BUFFER_DATA(pk_SceneEnv_SH, 0), direction);
    const float G = SH_EvaluateDiffuse(PK_BUFFER_DATA(pk_SceneEnv_SH, 1), direction);
    const float B = SH_EvaluateDiffuse(PK_BUFFER_DATA(pk_SceneEnv_SH, 2), direction);
    return float3(R,G,B);
}

float3 SceneEnv_Sample_SH_Volumetric(float3 view_dir, float phase) 
{ 
    const float4 zh = float4(1.0f, view_dir) * float4(1.0f, phase.xxx);
    return SceneEnv_Sample_SH(zh); 
}
