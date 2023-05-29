#pragma once
#ifndef PK_COMMON
#define PK_COMMON

#include Utilities.glsl
#include Constants.glsl

PK_DECLARE_CBUFFER(pk_PerFrameConstants, PK_SET_GLOBAL)
{
    float4 pk_Time;      // Time since load (t/20, t, t*2, t*3), use to animate things inside the shaders.
    float4 pk_SinTime;   // Sine of time: (t/8, t/4, t/2, t).
    float4 pk_CosTime;   // Cosine of time: (t/8, t/4, t/2, t).
    float4 pk_DeltaTime; // Delta time: (dt, 1/dt, smoothDt, 1/smoothDt).
    
    float4 pk_CursorParams;         // xy = cursor screen position, zw = cursor screen delta.
    float4 pk_WorldSpaceCameraPos;  // World space position of the camera.
    float4 pk_ViewSpaceCameraDelta; // View space delta position of the camera.
    float4 pk_ProjectionParams;     // x = n, y = f, z = f - n, w = 1.0f / f.
    float4 pk_ExpProjectionParams;  // x = 1.0f / log2(f / n), y = -log2(n) / log2(f / n), z = f / n, w = 1.0f / n.
    float4 pk_ScreenParams;         // xy = current screen (width, height), z = 1 / width, w = 1 / height.
    float4 pk_ShadowCascadeZSplits; // view space z axis splits for directional light shadow cascades
    float4 pk_ProjectionJitter;     // xy = sub pixel jitter, zw = previous frame jitter
    uint2 pk_ScreenSize;            // xy = current screen size
    uint2 pk_FrameIndex;            // x = frame index since load, y = frame index since resize

    float4x4 pk_MATRIX_V;       // Current view matrix.
    float4x4 pk_MATRIX_I_V;     // Current inverse view matrix.
    float4x4 pk_MATRIX_P;       // Current projection matrix.
    float4x4 pk_MATRIX_I_P;     // Current inverse projection matrix.
    float4x4 pk_MATRIX_VP;      // Current view * projection matrix.
    float4x4 pk_MATRIX_I_VP;    // Current inverse view * projection matrix.
    float4x4 pk_MATRIX_L_I_V;   // Last inverse view matrix.
    float4x4 pk_MATRIX_L_VP;    // Last view * projection matrix.
    float4x4 pk_MATRIX_LD_P;    // Last view * projection * current inverse view matrix.
    float pk_SceneOEM_Exposure; // Scene reflections exposure
};

#if !defined(PK_INSTANCING_ENABLED)
PK_DECLARE_CBUFFER(pk_ModelMatrices, PK_SET_DRAW)
{
    float4x4 pk_MATRIX_M; // Current model matrix.
    float4x4 pk_MATRIX_I_M; // Current inverse model matrix.
};
#endif

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenDepthCurrent;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenDepthPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenNormalsCurrent;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenNormalsPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_ScreenColorPrevious;
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_SceneOEM_HDR;
PK_DECLARE_ACCELERATION_STRUCTURE(PK_SET_SHADER, pk_SceneStructure)

//----------GBUFFER ENCODING UTILITIES----------//
// Source: https://aras-p.info/texts/CompactNormalStorage.html
float4 EncodeGBufferN(float3 normal, float roughness) { return float4(normal.xy / sqrt(-normal.z * 8 + 8) + 0.5, roughness, 0.0f); }

float4 DecodeGBufferN(float4 encoded)
{
    float2 fenc = encoded.wz * 4 - 2;
    float f = dot(fenc, fenc);
    return float4(fenc * sqrt(1 - f / 4), -(1 - f / 2), encoded.y);
}

float LinearizeDepth(float z) { return 1.0f / (pk_MATRIX_I_P[2][3] * (z * 2.0f - 1.0f) + pk_MATRIX_I_P[3][3]); } 
float4 LinearizeDepth(float4 z) { return 1.0f / (pk_MATRIX_I_P[2][3] * (z * 2.0f - 1.0f) + pk_MATRIX_I_P[3][3]); } 

uint GetShadowCascadeIndex(float linearDepth)
{
    return linearDepth > pk_ShadowCascadeZSplits[1] ? 
           linearDepth > pk_ShadowCascadeZSplits[2] ? 
           linearDepth > pk_ShadowCascadeZSplits[3] ? 3 : 2 : 1 : 0;
}

//----------GBUFFER SAMPLING----------//
float SampleLinearDepth(float2 uv) { return LinearizeDepth(tex2D(pk_ScreenDepthCurrent, uv).x); }
float SampleLinearDepth(int2 coord) { return LinearizeDepth(texelFetch(pk_ScreenDepthCurrent, coord, 0).x); }
#define SampleLinearDepthOffsets(uv, offsets) LinearizeDepth(textureGatherOffsets(pk_ScreenDepthCurrent, uv, offsets))

float SamplePreviousLinearDepth(float2 uv) { return LinearizeDepth(tex2D(pk_ScreenDepthPrevious, uv).x); }
float SamplePreviousLinearDepth(int2 coord) { return LinearizeDepth(texelFetch(pk_ScreenDepthPrevious, coord, 0).x); }
#define SamplePreviousLinearDepthOffsets(uv, offsets) LinearizeDepth(textureGatherOffsets(pk_ScreenDepthPrevious, uv, offsets))

float SampleRoughness(float2 uv) { return tex2D(pk_ScreenNormalsCurrent, uv).y; }
float SampleRoughness(int2 coord) { return texelFetch(pk_ScreenNormalsCurrent, coord, 0).y; }
float4 SampleViewNormalRoughness(float2 uv) { return DecodeGBufferN(tex2D(pk_ScreenNormalsCurrent, uv)); }
float4 SampleViewNormalRoughness(int2 coord) { return DecodeGBufferN(texelFetch(pk_ScreenNormalsCurrent, coord, 0)); }

float SamplePreviousRoughness(float2 uv) { return tex2D(pk_ScreenNormalsPrevious, uv).y; }
float SamplePreviousRoughness(int2 coord) { return texelFetch(pk_ScreenNormalsPrevious, coord, 0).y; }
float4 SamplePreviousViewNormalRoughness(float2 uv) { return DecodeGBufferN(tex2D(pk_ScreenNormalsPrevious, uv)); }
float4 SamplePreviousViewNormalRoughness(int2 coord) { return DecodeGBufferN(texelFetch(pk_ScreenNormalsPrevious, coord, 0)); }

//----------COORDINATE TRANSFORMS----------//
float4 WorldToClipPos( in float3 pos) { return mul(pk_MATRIX_VP, float4(pos, 1.0)); }
float4 ViewToClipPos( in float3 pos) { return mul(pk_MATRIX_P, float4(pos, 1.0)); }
float3 WorldToViewPos( in float3 pos) { return mul(pk_MATRIX_V, float4(pos, 1.0)).xyz; }
float3 ObjectToViewPos( in float3 pos) { return mul(pk_MATRIX_V, mul(pk_MATRIX_M, float4(pos, 1.0))).xyz; }
float3 ObjectToViewPos(float4 pos) { return ObjectToViewPos(pos.xyz); }
float3 ObjectToWorldPos( in float3 pos) { return mul(pk_MATRIX_M, float4(pos, 1.0)).xyz; }
float3 ObjectToWorldDir( in float3 dir) { return normalize(mul(float3x3(pk_MATRIX_M), dir)); }
float3 ObjectToWorldVector( in float3 dir) { return mul(float3x3(pk_MATRIX_M), dir); }
float3 ObjectToViewDir(float3 dir) { return normalize(mul(float3x3(pk_MATRIX_V), ObjectToWorldVector(dir))); }
float3 ObjectToWorldNormal( in float3 normal) { return normalize(mul(normal, float3x3(pk_MATRIX_I_M))); }
float3 WorldToObjectPos(in float3 pos) { return mul(pk_MATRIX_I_M, float4(pos, 1.0f)).xyz; }
float3 WorldToObjectVector( in float3 dir) { return mul(float3x3(pk_MATRIX_I_M), dir); }
float3 WorldToObjectDir( in float3 dir) { return normalize(mul(float3x3(pk_MATRIX_I_M), dir)); }
float3 WorldToViewDir(float3 dir) { return normalize(mul(float3x3(pk_MATRIX_V), dir)); }
float4 ObjectToClipPos( in float3 pos) { return mul(pk_MATRIX_VP, mul(pk_MATRIX_M, float4(pos, 1.0))); }
float4 ObjectToClipPos(float4 pos) { return ObjectToClipPos(pos.xyz); }

float2 ClampClipUVBorder(float2 uv) { return clamp(uv, 0.5f * pk_ScreenParams.zw, 1.0f.xx - pk_ScreenParams.zw * 0.5f); }
float3 ClipToViewPos(const float2 clipcoord, float linearDepth) { return float3(clipcoord * float2(pk_MATRIX_I_P[0][0], pk_MATRIX_I_P[1][1]), 1) * linearDepth; }
float3 ClipUVToViewPos(float2 uv, float linearDepth) { return ClipToViewPos(uv * 2 - 1, linearDepth); }
float3 ClipToViewPos(float3 clippos) { return ClipToViewPos(clippos.xy, LinearizeDepth(clippos.z)); }
float3 ClipToUVW(float4 clippos) { return (clippos.xyz / clippos.w) * 0.5f + 0.5f; }
float4 ClipToScreenPos(float4 clippos) { return float4(clippos.xy * 0.5f + clippos.w * 0.5f, clippos.zw); }
float3 WorldToClipUVW(float3 worldpos) { return ClipToUVW(WorldToClipPos(worldpos)); }

float3 ScreenToViewPos(float3 screenpos) { return ClipToViewPos(float3(screenpos.xy * pk_ScreenParams.zw.xy * 2.0f - 1.0f, screenpos.z)); }
float3 ScreenToViewPos(float2 screenpos, float linearDepth) { return ClipUVToViewPos(screenpos.xy * pk_ScreenParams.zw, linearDepth); }

float3x3 ComposeMikkTangentSpaceMatrix(float3 normal, float4 tangent)
{
    float3 T = normalize(tangent.xyz);
    float3 B = normalize(tangent.w * cross(normal, tangent.xyz));
    float3 N = normalize(normal);
    return mul(float3x3(pk_MATRIX_M), float3x3(T, B, N));
}

bool Test_DepthReproject(const float z, const float zprev, const float bias) { return (abs(z - zprev - pk_ViewSpaceCameraDelta.z) / z) < bias; }
bool Test_DepthFar(const float depth) { return depth < (pk_ProjectionParams.z - 1e-2f); }
bool Test_ClipPos(const float4 clippos) { return clippos.z > 0.0f && all(lessThan(abs(clippos.xy / clippos.w), 1.0f.xx)); }
bool Test_WorldToClipSpace(float3 worldpos) { return Test_ClipPos(WorldToClipPos(worldpos)); }

bool Test_ViewToClipUVW(float3 viewpos, inout float3 uvw)
{
    float4 clippos = ViewToClipPos(viewpos);
    uvw = ClipToUVW(clippos);
    return Test_ClipPos(clippos);
}

bool Test_WorldToClipUVW(float3 worldpos, inout float3 uvw)
{
    float4 clippos = WorldToClipPos(worldpos);
    uvw = ClipToUVW(clippos);
    return Test_ClipPos(clippos);
}

bool Test_WorldToPrevClipUVW(float3 worldpos, inout float3 uvw)
{
    float4 clippos = mul(pk_MATRIX_L_VP, float4(worldpos, 1.0f));
    uvw = ClipToUVW(clippos);
    return Test_ClipPos(clippos);
}

#endif