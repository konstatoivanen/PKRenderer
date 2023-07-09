#pragma once
#ifndef PK_GBUFFERS
#define PK_GBUFFERS

#include Common.glsl

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenDepthCurrent;
PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_ScreenDepthHierachical;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenDepthPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenNormalsCurrent;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenNormalsPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenColorPrevious;

// Source: https://aras-p.info/texts/CompactNormalStorage.html
float4 EncodeGBufferN(float3 normal, float roughness) { return float4(normal.xy / sqrt(-normal.z * 8 + 8) + 0.5, roughness, 0.0f); }

float4 DecodeGBufferN(float4 encoded)
{
    float2 fenc = encoded.wz * 4 - 2;
    float f = dot(fenc, fenc);
    return float4(fenc * sqrt(1 - f / 4), -(1 - f / 2), encoded.y);
}

float3 SamplePreviousColor(float2 uv) { return tex2D(pk_ScreenColorPrevious, uv).rgb; }
float3 SamplePreviousColor(int2 coord) { return texelFetch(pk_ScreenColorPrevious, coord, 0).rgb; }

float SampleMinZ(int2 coord, int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 0), l).x; }
float SampleMaxZ(int2 coord, int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 1), l).x; }
float SampleAvgZ(int2 coord, int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 2), l).x; }

float SampleMinZ(float2 uv, float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 0), l).x; }
float SampleMaxZ(float2 uv, float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 1), l).x; }
float SampleAvgZ(float2 uv, float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 2), l).x; }

float SampleViewDepth(float2 uv) { return ViewDepth(tex2D(pk_ScreenDepthCurrent, uv).x); }
float SampleViewDepth(int2 coord) { return ViewDepth(texelFetch(pk_ScreenDepthCurrent, coord, 0).x); }
#define GatherViewDepths(uv) ViewDepth(textureGather(pk_ScreenDepthCurrent, uv, 0))
#define SampleViewDepthOffsets(uv, offsets) ViewDepth(textureGatherOffsets(pk_ScreenDepthCurrent, uv, offsets))

float SamplePreviousViewDepth(float2 uv) { return ViewDepth(tex2D(pk_ScreenDepthPrevious, uv).x); }
float SamplePreviousViewDepth(int2 coord) { return ViewDepth(texelFetch(pk_ScreenDepthPrevious, coord, 0).x); }
#define SamplePreviousViewDepthOffsets(uv, offsets) ViewDepth(textureGatherOffsets(pk_ScreenDepthPrevious, uv, offsets))

float SampleRoughness(float2 uv) { return tex2D(pk_ScreenNormalsCurrent, uv).y; }
float SampleRoughness(int2 coord) { return texelFetch(pk_ScreenNormalsCurrent, coord, 0).y; }
float4 SampleViewNormalRoughness(float2 uv) { return DecodeGBufferN(tex2D(pk_ScreenNormalsCurrent, uv)); }
float4 SampleViewNormalRoughness(int2 coord) { return DecodeGBufferN(texelFetch(pk_ScreenNormalsCurrent, coord, 0)); }

float SamplePreviousRoughness(float2 uv) { return tex2D(pk_ScreenNormalsPrevious, uv).y; }
float SamplePreviousRoughness(int2 coord) { return texelFetch(pk_ScreenNormalsPrevious, coord, 0).y; }
float4 SamplePreviousViewNormalRoughness(float2 uv) { return DecodeGBufferN(tex2D(pk_ScreenNormalsPrevious, uv)); }
float4 SamplePreviousViewNormalRoughness(int2 coord) { return DecodeGBufferN(texelFetch(pk_ScreenNormalsPrevious, coord, 0)); }

float3 SampleViewNormal(float2 uv) { return SampleViewNormalRoughness(uv).xyz; }
float3 SampleViewNormal(int2 coord) { return SampleViewNormalRoughness(coord).xyz; }
float3 SampleWorldNormal(float2 uv) { return ViewToWorldDir(SampleViewNormal(uv)); }
float3 SampleWorldNormal(int2 coord) { return ViewToWorldDir(SampleViewNormal(coord)); }

float4 SampleWorldNormalRoughness(float2 uv) { return mul3x3(float3x3(pk_MATRIX_I_V), SampleViewNormalRoughness(uv)); }
float4 SampleWorldNormalRoughness(int2 coord) { return mul3x3(float3x3(pk_MATRIX_I_V), SampleViewNormalRoughness(coord)); }

float3 SampleViewPosition(float2 uv) { return UVToViewPos(uv, SampleViewDepth(uv)); }
float3 SampleViewPosition(float2 uv, float viewDepth) { return UVToViewPos(uv, viewDepth); }
float3 SampleViewPosition(int2 coord, int2 size) { return UVToViewPos((coord + 0.5f.xx) / float2(size), SampleViewDepth(coord)); }
float3 SampleViewPosition(int2 coord, int2 size, float viewDepth) { return UVToViewPos((coord + 0.5f.xx) / float2(size), viewDepth); }

float3 SampleWorldPosition(float2 uv) { return ViewToWorldPos(SampleViewPosition(uv)); }
float3 SampleWorldPosition(float2 uv, float viewDepth) { return ViewToWorldPos(SampleViewPosition(uv, viewDepth)); }
float3 SampleWorldPosition(int2 coord, int2 size) { return ViewToWorldPos(SampleViewPosition(coord, size)); }
float3 SampleWorldPosition(int2 coord, int2 size, float viewDepth) { return ViewToWorldPos(SampleViewPosition(coord, size, viewDepth)); }

float3 SamplePreviousViewNormal(float2 uv) { return SamplePreviousViewNormalRoughness(uv).xyz; }
float3 SamplePreviousViewNormal(int2 coord) { return SamplePreviousViewNormalRoughness(coord).xyz; }
float3 SamplePreviousWorldNormal(float2 uv) { return mul(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormal(uv)); }
float3 SamplePreviousWorldNormal(int2 coord) { return mul(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormal(coord)); }

float3 SamplePreviousViewPosition(float2 uv) { return UVToViewPos(uv, SamplePreviousViewDepth(uv)); }
float3 SamplePreviousViewPosition(int2 coord, int2 size) { return UVToViewPos((coord + 0.5f.xx) / float2(size), SamplePreviousViewDepth(coord)); }
float3 SamplePreviousWorldPosition(float2 uv) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(uv), 1.0f)).xyz; }
float3 SamplePreviousWorldPosition(int2 coord, int2 size) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(coord, size), 1.0f)).xyz; }

#endif