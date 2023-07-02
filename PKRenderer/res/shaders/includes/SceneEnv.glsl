#pragma once

#include Utilities.glsl
#include Constants.glsl

#define PK_SCENE_ENV_MAX_MIP 4
#define PK_SCENE_ENV_MIN_SIZE 32

// #define PK_SCENE_ENV_EXPOSURE required to defined before this include

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_SceneEnv;
PK_DECLARE_BUFFER(float4, pk_SceneEnv_SH, PK_SET_GLOBAL);

float3 SampleEnvironment(float2 uv, float roughness) 
{ 
    return tex2DLod(pk_SceneEnv, uv, roughness * PK_SCENE_ENV_MAX_MIP).rgb * PK_SCENE_ENV_EXPOSURE; 
}

float3 SampleEnvironmentSH(float4 basis)
{
    const float4 shR = PK_BUFFER_DATA(pk_SceneEnv_SH, 0);
    const float4 shG = PK_BUFFER_DATA(pk_SceneEnv_SH, 1);
    const float4 shB = PK_BUFFER_DATA(pk_SceneEnv_SH, 2);
    return float3(max(0.0, dot(shR, basis)), max(0.0, dot(shG, basis)), max(0.0, dot(shB, basis))) * PK_SCENE_ENV_EXPOSURE;
}

float3 SampleEnvironmentSH(float3 direction)  { return SampleEnvironmentSH(float4(1.0f, direction.yzx) * pk_L1Basis_Cosine * 2.0f);  }

float3 SampleEnvironmentSHVolumetric(float3 viewdir, float phase) 
{ 
    float4 zh = float4(1.0f, viewdir.yzx) * float4(1.0f, phase.xxx);
    return SampleEnvironmentSH(zh); 
}
