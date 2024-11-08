#pragma once

#include "Utilities.glsl"
#include "Constants.glsl"

#define PK_SCENE_ENV_MAX_MIP 4
#define PK_SCENE_ENV_MIN_SIZE 32

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_SceneEnv;
PK_DECLARE_BUFFER(float4, pk_SceneEnv_SH, PK_SET_GLOBAL);

float3 SampleEnvironment(float2 uv, float roughness) 
{ 
    return textureLod(pk_SceneEnv, uv, roughness * PK_SCENE_ENV_MAX_MIP).rgb; 
}

float3 SampleEnvironmentSH(float4 basis)
{
    const float R = max(0.0f, dot(PK_BUFFER_DATA(pk_SceneEnv_SH, 0), basis));
    const float G = max(0.0f, dot(PK_BUFFER_DATA(pk_SceneEnv_SH, 1), basis));
    const float B = max(0.0f, dot(PK_BUFFER_DATA(pk_SceneEnv_SH, 2), basis));
    return float3(R, G, B);
}

float3 SampleEnvironmentSHPeakDirection()
{
    float3 direction = 0.0f.xxx;
    direction += PK_BUFFER_DATA(pk_SceneEnv_SH, 0).yzw * PK_LUMA_BT709.r;
    direction += PK_BUFFER_DATA(pk_SceneEnv_SH, 1).yzw * PK_LUMA_BT709.g;
    direction += PK_BUFFER_DATA(pk_SceneEnv_SH, 2).yzw * PK_LUMA_BT709.b;
    return direction / (length(direction) + 1e-6f); 
}

float3 SampleEnvironmentSHColor()
{
    const float R = PK_BUFFER_DATA(pk_SceneEnv_SH, 0).x;
    const float G = PK_BUFFER_DATA(pk_SceneEnv_SH, 1).x;
    const float B = PK_BUFFER_DATA(pk_SceneEnv_SH, 2).x;
    return float3(R, G, B) / PK_L1BASIS.xxx;
}

float3 SampleEnvironmentSHDiffuse(float3 direction)  { return SampleEnvironmentSH(float4(1.0f, direction) * PK_L1BASIS_COSINE * 2.0f);  }

float3 SampleEnvironmentSHVolumetric(float3 viewdir, float phase) 
{ 
    const float4 zh = float4(1.0f, viewdir) * float4(1.0f, phase.xxx);
    return SampleEnvironmentSH(zh); 
}
