#pragma once
#include Common.glsl
#include Encoding.glsl

float3 SampleEnvironment(float2 uv, float roughness) { return HDRDecode(tex2DLod(pk_SceneOEM_HDR, uv, roughness * 4)) * pk_SceneOEM_Exposure; }
