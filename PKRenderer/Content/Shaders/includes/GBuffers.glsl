#pragma once
#ifndef PK_GBUFFERS
#define PK_GBUFFERS

#include "Common.glsl"

PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Current_Normals;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Current_Depth;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Current_DepthBiased;
PK_DECLARE_SET_GLOBAL uniform texture2DArray pk_GB_Current_DepthMips;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Previous_Color;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Previous_Normals;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Previous_Depth;
PK_DECLARE_SET_GLOBAL uniform texture2D pk_GB_Previous_DepthBiased;

#define GBUFFER_SAMPLE(t, uv) texture(sampler2D(t, pk_Sampler_GBuffer), uv)
#define GBUFFER_SAMPLE_OFFSET(t, uv, offs) textureOffset(sampler2D(t, pk_Sampler_GBuffer), uv, offs)
#define GBUFFER_GATHER(t, uv, cmp) textureGather(sampler2D(t, pk_Sampler_GBuffer), uv, cmp)
#define GBUFFER_GATHER_OFFSETS(t, uv, offs) textureGatherOffsets(sampler2D(t, pk_Sampler_GBuffer), uv, offs)
#define GBUFFER_SMP_ARR_LOD(t, uv, l) textureLod(sampler2DArray(t, pk_Sampler_GBuffer), uv, l)
#define GBUFFER_NORMALS_10BIT 1

// Source: https://aras-p.info/texts/CompactNormalStorage.html
float4 EncodeGBufferViewNR(const float3 normal, const float roughness, const float metallic) 
{ 
    float2 xy = normal.xy / sqrt(-normal.z * 8 + 8) + 0.5f;

    #if defined(SHADER_STAGE_FRAGMENT) && GBUFFER_NORMALS_10BIT == 1
        // Low precision normals cause visible snapping. 
        // Dither them by generating some deterministic triangle noise.
        const float n = fract(52.9829189f * fract(0.06711056f * gl_FragCoord.x + 0.00583715f * gl_FragCoord.y));
        const float d = n * 2.0f - 1.0f;
        const float t = max(-1.0f, d * inversesqrt(abs(d))) - sign(d) + 0.5f;
        xy = saturate(xy + t * 9.775171065493646e-4f); // t / 1023.0f
    #endif

    return float4(xy, roughness, metallic); 
}

float4 EncodeGBufferWorldNR(const float3 normal, const float roughness, const float metallic)
{
    return EncodeGBufferViewNR(normalize(WorldToViewVec(normal)), roughness, metallic);
}

float4 DecodeGBufferViewNR(const float4 encoded)
{
    float2 fenc = encoded.wz * 4 - 2;
    float f = dot(fenc, fenc);
    return float4(fenc * sqrt(1 - f / 4), -(1 - f / 2), encoded.y);
}

float EncodeBiasedDepth(float clip_depth, float factor, float bias)
{
    const float view_depth = ViewDepth(clip_depth);

    bias *= 0.32f;
    bias *= exp(-factor * 0.5f);
    bias /= clip_depth + 0.01f;

    const float view_depth_biased = max(pk_ClipParams.x, view_depth - bias);
    float clip_depth_biased = ClipDepth(view_depth_biased);
    // Only allow bias towards camera as forward bias will cause clipping issues.
    //return clipDepth;
    return max(clip_depth, clip_depth_biased);
}

float3 SamplePreviousColor(const float2 uv) { return GBUFFER_SAMPLE(pk_GB_Previous_Color, uv).rgb; }
float3 SamplePreviousColor(const int2 coord) { return texelFetch(pk_GB_Previous_Color, coord, 0).rgb; }

float SampleMinZ(const int2 coord, const int l) { return texelFetch(pk_GB_Current_DepthMips, int3(coord, 0), l).x; }
float SampleMaxZ(const int2 coord, const int l) { return texelFetch(pk_GB_Current_DepthMips, int3(coord, 1), l).x; }
float SampleHiZ(const int2 coord, const int mode, const int l) { return texelFetch(pk_GB_Current_DepthMips, int3(coord, mode), l).x; }

float SampleMinZ(const float2 uv, const float l) { return GBUFFER_SMP_ARR_LOD(pk_GB_Current_DepthMips, float3(uv, 0), l).x; }
float SampleMaxZ(const float2 uv, const float l) { return GBUFFER_SMP_ARR_LOD(pk_GB_Current_DepthMips, float3(uv, 1), l).x; }
float SampleHiZ(const float2 uv, const int mode, const float l) { return GBUFFER_SMP_ARR_LOD(pk_GB_Current_DepthMips, float3(uv, mode), l).x; }

float SampleClipDepthBiased(const float2 uv) { return GBUFFER_SAMPLE(pk_GB_Current_DepthBiased, uv).x; }
float SampleClipDepthBiased(const int2 coord) { return texelFetch(pk_GB_Current_DepthBiased, coord, 0).x; }
float SampleViewDepthBiased(const float2 uv) { return ViewDepth(GBUFFER_SAMPLE(pk_GB_Current_DepthBiased, uv).x); }
float SampleViewDepthBiased(const int2 coord) { return ViewDepth(texelFetch(pk_GB_Current_DepthBiased, coord, 0).x); }

float SampleClipDepth(const float2 uv) { return GBUFFER_SAMPLE(pk_GB_Current_Depth, uv).x; }
float SampleClipDepth(const int2 coord) { return texelFetch(pk_GB_Current_Depth, coord, 0).x; }
float SampleViewDepth(const float2 uv) { return ViewDepth(GBUFFER_SAMPLE(pk_GB_Current_Depth, uv).x); }
float SampleViewDepth(const int2 coord) { return ViewDepth(texelFetch(pk_GB_Current_Depth, coord, 0).x); }

// Gather order: (0,1), (1,1), (1,0), (0,0) 
#define GatherViewDepths(uv) ViewDepth(GBUFFER_GATHER(pk_GB_Current_Depth, uv, 0))
#define GatherViewDepthsBiased(uv) ViewDepth(GBUFFER_GATHER(pk_GB_Current_DepthBiased, uv, 0))
#define SampleViewDepthOffsets(uv, offsets) ViewDepth(GBUFFER_GATHER_OFFSETS(pk_GB_Current_Depth, uv, offsets))

float SamplePreviousClipDepthBiased(const int2 coord) { return texelFetch(pk_GB_Previous_Depth, coord, 0).x; }
float SamplePreviousViewDepthBiased(const float2 uv) { return ViewDepth(GBUFFER_SAMPLE(pk_GB_Previous_Depth, uv).x); }
float SamplePreviousViewDepthBiased(const int2 coord) { return ViewDepth(texelFetch(pk_GB_Previous_Depth, coord, 0).x); }

float SamplePreviousClipDepth(const int2 coord) { return texelFetch(pk_GB_Previous_Depth, coord, 0).x; }
float SamplePreviousViewDepth(const float2 uv) { return ViewDepth(GBUFFER_SAMPLE(pk_GB_Previous_Depth, uv).x); }
float SamplePreviousViewDepth(const int2 coord) { return ViewDepth(texelFetch(pk_GB_Previous_Depth, coord, 0).x); }

#define GatherPreviousViewDepths(uv) ViewDepth(GBUFFER_GATHER(pk_GB_Previous_Depth, uv, 0))
#define GatherPreviousViewDepthsBiased(uv) ViewDepth(GBUFFER_GATHER(pk_GB_Previous_DepthBiased, uv, 0))
#define SamplePreviousViewDepthOffsets(uv, offsets) ViewDepth(GBUFFER_GATHER_OFFSETS(pk_GB_Previous_Depth, uv, offsets))

float SamplePreviousRoughness(const float2 uv) { return GBUFFER_SAMPLE(pk_GB_Previous_Normals, uv).y; }
float SamplePreviousRoughness(const int2 coord) { return texelFetch(pk_GB_Previous_Normals, coord, 0).y; }
float4 SamplePreviousViewNormalRoughness(const float2 uv) { return DecodeGBufferViewNR(GBUFFER_SAMPLE(pk_GB_Previous_Normals, uv)); }
float4 SamplePreviousViewNormalRoughness(const int2 coord) { return DecodeGBufferViewNR(texelFetch(pk_GB_Previous_Normals, coord, 0)); }
float4 SamplePreviousWorldNormalRoughness(const float2 uv) { return Mul3x3(SamplePreviousViewNormalRoughness(uv), float3x3(pk_ViewToWorldPrev)); }
float4 SamplePreviousWorldNormalRoughness(const int2 coord) { return Mul3x3(SamplePreviousViewNormalRoughness(coord), float3x3(pk_ViewToWorldPrev)); }
float3 SamplePreviousViewNormal(const float2 uv) { return SamplePreviousViewNormalRoughness(uv).xyz; }
float3 SamplePreviousViewNormal(const int2 coord) { return SamplePreviousViewNormalRoughness(coord).xyz; }
float3 SamplePreviousWorldNormal(const float2 uv) { return SamplePreviousViewNormal(uv) * float3x3(pk_ViewToWorldPrev); }
float3 SamplePreviousWorldNormal(const int2 coord) { return SamplePreviousViewNormal(coord) * float3x3(pk_ViewToWorldPrev); }

float SampleRoughness(const float2 uv) { return GBUFFER_SAMPLE(pk_GB_Current_Normals, uv).y; }
float SampleRoughness(const int2 coord) { return texelFetch(pk_GB_Current_Normals, coord, 0).y; }
float4 SampleViewNormalRoughness(const float2 uv) { return DecodeGBufferViewNR(GBUFFER_SAMPLE(pk_GB_Current_Normals, uv)); }
float4 SampleViewNormalRoughness(const int2 coord) { return DecodeGBufferViewNR(texelFetch(pk_GB_Current_Normals, coord, 0)); }
float4 SampleWorldNormalRoughness(const float2 uv) { return Mul3x3(SampleViewNormalRoughness(uv), float3x3(pk_ViewToWorld)); }
float4 SampleWorldNormalRoughness(const int2 coord) { return Mul3x3(SampleViewNormalRoughness(coord), float3x3(pk_ViewToWorld)); }
float3 SampleViewNormal(const float2 uv) { return SampleViewNormalRoughness(uv).xyz; }
float3 SampleViewNormal(const int2 coord) { return SampleViewNormalRoughness(coord).xyz; }
float3 SampleWorldNormal(const float2 uv) { return ViewToWorldVec(SampleViewNormal(uv)); }
float3 SampleWorldNormal(const int2 coord) { return ViewToWorldVec(SampleViewNormal(coord)); }

#endif
