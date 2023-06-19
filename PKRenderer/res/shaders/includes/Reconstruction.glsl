#pragma once
#ifndef PK_RECONSTRUCTION
#define PK_RECONSTRUCTION

#include Common.glsl

float3 UnpackNormal(in float3 packedNormal) { return packedNormal * 2.0f - float3(1.0f); }
float3 SampleNormal(in sampler2D map, in float3x3 rotation, in float2 uv, float amount) { return normalize(mul(rotation, lerp(float3(0,0,1), UnpackNormal(tex2D(map, uv).xyz), amount))); }
float2 ParallaxOffset(float height, float heightAmount, float3 viewdir) { return (height * heightAmount - heightAmount / 2.0f) * (viewdir.xy / (viewdir.z + 0.42f)); }

float3 SampleViewNormal(float2 uv) { return SampleViewNormalRoughness(uv).xyz; }
float3 SampleViewNormal(int2 coord) { return SampleViewNormalRoughness(coord).xyz; }
float3 SampleWorldNormal(float2 uv) { return mul(float3x3(pk_MATRIX_I_V), SampleViewNormal(uv)); }
float3 SampleWorldNormal(int2 coord) { return mul(float3x3(pk_MATRIX_I_V), SampleViewNormal(coord)); }

float4 SampleWorldNormalRoughness(float2 uv) { return mul3x3(float3x3(pk_MATRIX_I_V), SampleViewNormalRoughness(uv)); }
float4 SampleWorldNormalRoughness(int2 coord) { return mul3x3(float3x3(pk_MATRIX_I_V), SampleViewNormalRoughness(coord)); }

float3 SampleViewPosition(float2 uv) { return ClipUVToViewPos(uv, SampleLinearDepth(uv)); }
float3 SampleViewPosition(float2 uv, float linearDepth) { return ClipUVToViewPos(uv, linearDepth); }
float3 SampleViewPosition(int2 coord, int2 size) { return ClipUVToViewPos((coord + 0.5f.xx) / float2(size), SampleLinearDepth(coord)); }
float3 SampleViewPosition(int2 coord, int2 size, float linearDepth) { return ClipUVToViewPos((coord + 0.5f.xx) / float2(size), linearDepth); }

float3 SampleWorldPosition(float2 uv) { return mul(pk_MATRIX_I_V, float4(SampleViewPosition(uv), 1.0f)).xyz; }
float3 SampleWorldPosition(float2 uv, float linearDepth) { return mul(pk_MATRIX_I_V, float4(SampleViewPosition(uv, linearDepth), 1.0f)).xyz; }
float3 SampleWorldPosition(int2 coord, int2 size) { return mul(pk_MATRIX_I_V, float4(SampleViewPosition(coord, size), 1.0f)).xyz; }
float3 SampleWorldPosition(int2 coord, int2 size, float linearDepth) { return mul(pk_MATRIX_I_V, float4(SampleViewPosition(coord, size, linearDepth), 1.0f)).xyz; }

float3 SamplePreviousViewNormal(float2 uv) { return SamplePreviousViewNormalRoughness(uv).xyz; }
float3 SamplePreviousViewNormal(int2 coord) { return SamplePreviousViewNormalRoughness(coord).xyz; }
float3 SamplePreviousWorldNormal(float2 uv) { return mul(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormal(uv)); }
float3 SamplePreviousWorldNormal(int2 coord) { return mul(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormal(coord)); }

float3 SamplePreviousViewPosition(float2 uv) { return ClipUVToViewPos(uv, SamplePreviousLinearDepth(uv)); }
float3 SamplePreviousViewPosition(int2 coord, int2 size) { return ClipUVToViewPos((coord + 0.5f.xx) / float2(size), SamplePreviousLinearDepth(coord)); }
float3 SamplePreviousWorldPosition(float2 uv) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(uv), 1.0f)).xyz; }
float3 SamplePreviousWorldPosition(int2 coord, int2 size) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(coord, size), 1.0f)).xyz; }

float3 GetFragmentClipUVW()
{
    #if defined(SHADER_STAGE_FRAGMENT)
        return float3(gl_FragCoord.xy * pk_ScreenParams.zw, gl_FragCoord.z);
    #else
        return 0.0f.xxx;
    #endif
}

#endif