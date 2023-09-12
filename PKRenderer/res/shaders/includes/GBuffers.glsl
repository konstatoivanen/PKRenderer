#pragma once
#ifndef PK_GBUFFERS
#define PK_GBUFFERS

#include Common.glsl

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenDepthCurrent;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenDepthPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenNormalsCurrent;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenNormalsPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenColorPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_ScreenDepthHierachical;

// Source: https://aras-p.info/texts/CompactNormalStorage.html
float4 EncodeGBufferViewNR(const float3 normal, const float roughness) 
{ 
    return float4(normal.xy / sqrt(-normal.z * 8 + 8) + 0.5, roughness, 0.0f); 
}

float4 EncodeGBufferWorldNR(const float3 normal, const float roughness)
{
    return EncodeGBufferViewNR(normalize(WorldToViewDir(normal)), roughness);
}

float4 DecodeGBufferViewNR(const float4 encoded)
{
    float2 fenc = encoded.wz * 4 - 2;
    float f = dot(fenc, fenc);
    return float4(fenc * sqrt(1 - f / 4), -(1 - f / 2), encoded.y);
}

float3 SamplePreviousColor(const float2 uv) { return tex2D(pk_ScreenColorPrevious, uv).rgb; }
float3 SamplePreviousColor(const int2 coord) { return texelFetch(pk_ScreenColorPrevious, coord, 0).rgb; }

float SampleMinZ(const int2 coord, const int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 0), l).x; }
float SampleMaxZ(const int2 coord, const int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 1), l).x; }
float SampleAvgZ(const int2 coord, const int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 2), l).x; }

float SampleMinZ(const float2 uv, const float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 0), l).x; }
float SampleMaxZ(const float2 uv, const float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 1), l).x; }
float SampleAvgZ(const float2 uv, const float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 2), l).x; }

float SampleViewDepth(const float2 uv) { return ViewDepth(tex2D(pk_ScreenDepthCurrent, uv).x); }
float SampleViewDepth(const int2 coord) { return ViewDepth(texelFetch(pk_ScreenDepthCurrent, coord, 0).x); }
#define GatherViewDepths(uv) ViewDepth(textureGather(pk_ScreenDepthCurrent, uv, 0))
#define SampleViewDepthOffsets(uv, offsets) ViewDepth(textureGatherOffsets(pk_ScreenDepthCurrent, uv, offsets))

float SamplePreviousViewDepth(const float2 uv) { return ViewDepth(tex2D(pk_ScreenDepthPrevious, uv).x); }
float SamplePreviousViewDepth(const int2 coord) { return ViewDepth(texelFetch(pk_ScreenDepthPrevious, coord, 0).x); }
#define GatherPreviousViewDepths(uv) ViewDepth(textureGather(pk_ScreenDepthPrevious, uv, 0))
#define SamplePreviousViewDepthOffsets(uv, offsets) ViewDepth(textureGatherOffsets(pk_ScreenDepthPrevious, uv, offsets))

float SampleRoughness(const float2 uv) { return tex2D(pk_ScreenNormalsCurrent, uv).y; }
float SampleRoughness(const int2 coord) { return texelFetch(pk_ScreenNormalsCurrent, coord, 0).y; }
float4 SampleViewNormalRoughness(const float2 uv) { return DecodeGBufferViewNR(tex2D(pk_ScreenNormalsCurrent, uv)); }
float4 SampleViewNormalRoughness(const int2 coord) { return DecodeGBufferViewNR(texelFetch(pk_ScreenNormalsCurrent, coord, 0)); }

float SamplePreviousRoughness(const float2 uv) { return tex2D(pk_ScreenNormalsPrevious, uv).y; }
float SamplePreviousRoughness(const int2 coord) { return texelFetch(pk_ScreenNormalsPrevious, coord, 0).y; }
float4 SamplePreviousViewNormalRoughness(const float2 uv) { return DecodeGBufferViewNR(tex2D(pk_ScreenNormalsPrevious, uv)); }
float4 SamplePreviousViewNormalRoughness(const int2 coord) { return DecodeGBufferViewNR(texelFetch(pk_ScreenNormalsPrevious, coord, 0)); }
float4 SamplePreviousWorldNormalRoughness(const float2 uv) { return mul3x3(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormalRoughness(uv)); }
float4 SamplePreviousWorldNormalRoughness(const int2 coord) { return mul3x3(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormalRoughness(coord)); }

float3 SampleViewNormal(const float2 uv) { return SampleViewNormalRoughness(uv).xyz; }
float3 SampleViewNormal(const int2 coord) { return SampleViewNormalRoughness(coord).xyz; }
float3 SampleWorldNormal(const float2 uv) { return ViewToWorldDir(SampleViewNormal(uv)); }
float3 SampleWorldNormal(const int2 coord) { return ViewToWorldDir(SampleViewNormal(coord)); }

float4 SampleWorldNormalRoughness(const float2 uv) { return mul3x3(float3x3(pk_MATRIX_I_V), SampleViewNormalRoughness(uv)); }
float4 SampleWorldNormalRoughness(const int2 coord) { return mul3x3(float3x3(pk_MATRIX_I_V), SampleViewNormalRoughness(coord)); }

float3 SampleViewPosition(const float2 uv) { return UVToViewPos(uv, SampleViewDepth(uv)); }
float3 SampleViewPosition(const float2 uv, float viewDepth) { return UVToViewPos(uv, viewDepth); }
float3 SampleViewPosition(const int2 coord) { return UVToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, SampleViewDepth(coord)); }
float3 SampleViewPosition(const int2 coord, const float viewDepth) { return UVToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, viewDepth); }

float3 SampleWorldPosition(const float2 uv) { return ViewToWorldPos(SampleViewPosition(uv)); }
float3 SampleWorldPosition(const float2 uv, float viewDepth) { return ViewToWorldPos(SampleViewPosition(uv, viewDepth)); }
float3 SampleWorldPosition(const int2 coord) { return ViewToWorldPos(SampleViewPosition(coord)); }
float3 SampleWorldPosition(const int2 coord, const float viewDepth) { return ViewToWorldPos(SampleViewPosition(coord, viewDepth)); }

float3 SamplePreviousViewNormal(const float2 uv) { return SamplePreviousViewNormalRoughness(uv).xyz; }
float3 SamplePreviousViewNormal(const int2 coord) { return SamplePreviousViewNormalRoughness(coord).xyz; }
float3 SamplePreviousWorldNormal(const float2 uv) { return mul(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormal(uv)); }
float3 SamplePreviousWorldNormal(const int2 coord) { return mul(float3x3(pk_MATRIX_L_I_V), SamplePreviousViewNormal(coord)); }

float3 SamplePreviousViewPosition(const float2 uv) { return UVToViewPos(uv, SamplePreviousViewDepth(uv)); }
float3 SamplePreviousViewPosition(const int2 coord) { return UVToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, SamplePreviousViewDepth(coord)); }
float3 SamplePreviousViewPosition(const int2 coord, const float viewDepth) { return UVToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, viewDepth); }
float3 SamplePreviousWorldPosition(const float2 uv) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(uv), 1.0f)).xyz; }
float3 SamplePreviousWorldPosition(const int2 coord) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(coord), 1.0f)).xyz; }
float3 SamplePreviousWorldPosition(const int2 coord, const float viewDepth) { return mul(pk_MATRIX_L_I_V, float4(SamplePreviousViewPosition(coord, viewDepth), 1.0f)).xyz; }

#endif