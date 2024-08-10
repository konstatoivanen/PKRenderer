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

float3 SampleEnvironmentSH(float3 direction)  { return SampleEnvironmentSH(float4(1.0f, direction.yzx) * PK_L1BASIS_COSINE * 2.0f);  }

float3 SampleEnvironmentSHVolumetric(float3 viewdir, float phase) 
{ 
    float4 zh = float4(1.0f, viewdir.yzx) * float4(1.0f, phase.xxx);
    return SampleEnvironmentSH(zh); 
}
