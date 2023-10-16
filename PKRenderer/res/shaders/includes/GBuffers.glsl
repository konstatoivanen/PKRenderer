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

#define GBUFFER_NORMALS_10BIT 1

// Source: https://aras-p.info/texts/CompactNormalStorage.html
float4 EncodeGBufferViewNR(const float3 normal, const float roughness) 
{ 
    float2 xy = normal.xy / sqrt(-normal.z * 8 + 8) + 0.5f;

    #if defined(SHADER_STAGE_FRAGMENT) && GBUFFER_NORMALS_10BIT == 1
        // Low precision normals cause visible snapping. 
        // Dither them by generating some deterministic triangle noise.
        uint2 px = uint2(gl_FragCoord.xy);
        px = (px | (px << 8)) & 0x00FF00FF;
        px = (px | (px << 4)) & 0x0F0F0F0F;
        px = (px | (px << 2)) & 0x33333333;
        px = (px | (px << 1)) & 0x55555555;
        uint hash = px.x | (px.y << 1);
        hash ^= hash >> 16; 
        hash *= 0x7feb352dU;
        hash ^= hash >> 15; 
        hash *= 0x846ca68bU;
        hash ^= hash >> 16;
        const float d = uintBitsToFloat(hash & 0x007fffffu | 0x3f800000u) * 2.0f - 3.0f;
	    const float t = max(-1.0f, d * inversesqrt(abs(d))) - sign(d) + 0.5f;
        xy = saturate(xy + t * 9.775171065493646e-4f); // t / 1023.0f
    #endif

    return float4(xy, roughness, 0.0f); 
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

float3 SamplePreviousColor(const float2 uv) { return texture(pk_ScreenColorPrevious, uv).rgb; }
float3 SamplePreviousColor(const int2 coord) { return texelFetch(pk_ScreenColorPrevious, coord, 0).rgb; }

float SampleMinZ(const int2 coord, const int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 0), l).x; }
float SampleMaxZ(const int2 coord, const int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 1), l).x; }
float SampleAvgZ(const int2 coord, const int l) { return texelFetch(pk_ScreenDepthHierachical, int3(coord, 2), l).x; }

float SampleMinZ(const float2 uv, const float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 0), l).x; }
float SampleMaxZ(const float2 uv, const float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 1), l).x; }
float SampleAvgZ(const float2 uv, const float l) { return textureLod(pk_ScreenDepthHierachical, float3(uv, 2), l).x; }

float SampleViewDepth(const float2 uv) { return ViewDepth(texture(pk_ScreenDepthCurrent, uv).x); }
float SampleViewDepth(const int2 coord) { return ViewDepth(texelFetch(pk_ScreenDepthCurrent, coord, 0).x); }
// Gather order: (0,1), (1,1), (1,0), (0,0) 
#define GatherViewDepths(uv) ViewDepth(textureGather(pk_ScreenDepthCurrent, uv, 0))
#define SampleViewDepthOffsets(uv, offsets) ViewDepth(textureGatherOffsets(pk_ScreenDepthCurrent, uv, offsets))

float SamplePreviousViewDepth(const float2 uv) { return ViewDepth(texture(pk_ScreenDepthPrevious, uv).x); }
float SamplePreviousViewDepth(const int2 coord) { return ViewDepth(texelFetch(pk_ScreenDepthPrevious, coord, 0).x); }
#define GatherPreviousViewDepths(uv) ViewDepth(textureGather(pk_ScreenDepthPrevious, uv, 0))
#define SamplePreviousViewDepthOffsets(uv, offsets) ViewDepth(textureGatherOffsets(pk_ScreenDepthPrevious, uv, offsets))

float SampleRoughness(const float2 uv) { return texture(pk_ScreenNormalsCurrent, uv).y; }
float SampleRoughness(const int2 coord) { return texelFetch(pk_ScreenNormalsCurrent, coord, 0).y; }
float4 SampleViewNormalRoughness(const float2 uv) { return DecodeGBufferViewNR(texture(pk_ScreenNormalsCurrent, uv)); }
float4 SampleViewNormalRoughness(const int2 coord) { return DecodeGBufferViewNR(texelFetch(pk_ScreenNormalsCurrent, coord, 0)); }

float SamplePreviousRoughness(const float2 uv) { return texture(pk_ScreenNormalsPrevious, uv).y; }
float SamplePreviousRoughness(const int2 coord) { return texelFetch(pk_ScreenNormalsPrevious, coord, 0).y; }
float4 SamplePreviousViewNormalRoughness(const float2 uv) { return DecodeGBufferViewNR(texture(pk_ScreenNormalsPrevious, uv)); }
float4 SamplePreviousViewNormalRoughness(const int2 coord) { return DecodeGBufferViewNR(texelFetch(pk_ScreenNormalsPrevious, coord, 0)); }
float4 SamplePreviousWorldNormalRoughness(const float2 uv) { return mul3x3(SamplePreviousViewNormalRoughness(uv), float3x3(pk_ViewToWorldPrev)); }
float4 SamplePreviousWorldNormalRoughness(const int2 coord) { return mul3x3(SamplePreviousViewNormalRoughness(coord), float3x3(pk_ViewToWorldPrev)); }

float3 SampleViewNormal(const float2 uv) { return SampleViewNormalRoughness(uv).xyz; }
float3 SampleViewNormal(const int2 coord) { return SampleViewNormalRoughness(coord).xyz; }
float3 SampleWorldNormal(const float2 uv) { return ViewToWorldDir(SampleViewNormal(uv)); }
float3 SampleWorldNormal(const int2 coord) { return ViewToWorldDir(SampleViewNormal(coord)); }

float4 SampleWorldNormalRoughness(const float2 uv) { return mul3x3(SampleViewNormalRoughness(uv), float3x3(pk_ViewToWorld)); }
float4 SampleWorldNormalRoughness(const int2 coord) { return mul3x3(SampleViewNormalRoughness(coord), float3x3(pk_ViewToWorld)); }

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
float3 SamplePreviousWorldNormal(const float2 uv) { return mul(SamplePreviousViewNormal(uv), float3x3(pk_ViewToWorldPrev)); }
float3 SamplePreviousWorldNormal(const int2 coord) { return mul(SamplePreviousViewNormal(coord), float3x3(pk_ViewToWorldPrev)); }

float3 SamplePreviousViewPosition(const float2 uv) { return UVToViewPos(uv, SamplePreviousViewDepth(uv)); }
float3 SamplePreviousViewPosition(const int2 coord) { return UVToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, SamplePreviousViewDepth(coord)); }
float3 SamplePreviousViewPosition(const int2 coord, const float viewDepth) { return UVToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, viewDepth); }
float3 SamplePreviousWorldPosition(const float2 uv) { return mul(float4(SamplePreviousViewPosition(uv), 1.0f), pk_ViewToWorldPrev).xyz; }
float3 SamplePreviousWorldPosition(const int2 coord) { return mul(float4(SamplePreviousViewPosition(coord), 1.0f), pk_ViewToWorldPrev).xyz; }
float3 SamplePreviousWorldPosition(const int2 coord, const float viewDepth) { return mul(float4(SamplePreviousViewPosition(coord, viewDepth), 1.0f), pk_ViewToWorldPrev).xyz; }

#endif