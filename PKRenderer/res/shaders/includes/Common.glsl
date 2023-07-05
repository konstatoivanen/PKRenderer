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
    uint4 pk_FrameRandom;           // Random uint4 values for current frame.
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
    float pk_SceneEnv_Exposure; // Scene background environment exposure
};

#define PK_SCENE_ENV_EXPOSURE pk_SceneEnv_Exposure

#if !defined(PK_INSTANCING_ENABLED)
PK_DECLARE_CBUFFER(pk_ModelMatrices, PK_SET_DRAW)
{
    float4x4 pk_MATRIX_M; // Current model matrix.
    float4x4 pk_MATRIX_I_M; // Current inverse model matrix.
};
#endif

PK_DECLARE_ACCELERATION_STRUCTURE(PK_SET_SHADER, pk_SceneStructure)

uint GetShadowCascadeIndex(float linearDepth)
{
    return linearDepth > pk_ShadowCascadeZSplits[1] ? 
           linearDepth > pk_ShadowCascadeZSplits[2] ? 
           linearDepth > pk_ShadowCascadeZSplits[3] ? 3 : 2 : 1 : 0;
}

//----------TRANSFORMS----------//
float3 ObjectToWorldPos(const float3 pos) { return mul(pk_MATRIX_M, float4(pos, 1.0)).xyz; }
float3 ObjectToWorldDir(const float3 dir) { return mul(float3x3(pk_MATRIX_M), dir); }
float3 ObjectToViewPos(const float3 pos) { return mul(pk_MATRIX_V, mul(pk_MATRIX_M, float4(pos, 1.0))).xyz; }
float3 ObjectToViewDir(const float3 dir) { return mul(float3x3(pk_MATRIX_V), ObjectToWorldDir(dir)); }
float4 ObjectToClipPos(const float3 pos) { return mul(pk_MATRIX_VP, mul(pk_MATRIX_M, float4(pos, 1.0))); }

float3 WorldToObjectPos(const float3 pos) { return mul(pk_MATRIX_I_M, float4(pos, 1.0f)).xyz; }
float3 WorldToObjectDir(const float3 dir) { return mul(float3x3(pk_MATRIX_I_M), dir); }
float3 WorldToViewPos(const float3 pos) { return mul(pk_MATRIX_V, float4(pos, 1.0)).xyz; }
float3 WorldToViewDir(const float3 dir) { return mul(float3x3(pk_MATRIX_V), dir); }
float4 WorldToClipPos(const float3 pos) { return mul(pk_MATRIX_VP, float4(pos, 1.0)); }
float4 WorldToClipDir(const float3 dir) { return mul(pk_MATRIX_VP, float4(dir, 0.0)); }

float4 ViewToClipPos(const float3 pos) { return mul(pk_MATRIX_P, float4(pos, 1.0)); }
float4 ViewToClipDir(const float3 dir) { return mul(pk_MATRIX_P, float4(dir, 0.0)); }
float3 ViewToWorldPos(const float3 pos) { return mul(pk_MATRIX_I_V, float4(pos, 1.0f)).xyz; }
float3 ViewToWorldDir(const float3 dir) { return mul(float3x3(pk_MATRIX_I_V), dir); }

float LinearizeDepth(const float z) { return 1.0f / (pk_MATRIX_I_P[2][3] * (z * 2.0f - 1.0f) + pk_MATRIX_I_P[3][3]); } 
float4 LinearizeDepth(const float4 z) { return 1.0f / (pk_MATRIX_I_P[2][3] * (z * 2.0f - 1.0f) + pk_MATRIX_I_P[3][3]); } 
float ProjectDepth(const float z) { return 0.5f * (z * (pk_MATRIX_I_P[2][3] - pk_MATRIX_I_P[3][3]) + 1.0f) / (z * pk_MATRIX_I_P[2][3]); }
float4 ProjectDepth(const float4 z) { return 0.5f * (z * (pk_MATRIX_I_P[2][3] - pk_MATRIX_I_P[3][3]) + 1.0f) / (z * pk_MATRIX_I_P[2][3]); }

float2 ClampClipUVBorder(const float2 uv) { return clamp(uv, 0.5f * pk_ScreenParams.zw, 1.0f.xx - pk_ScreenParams.zw * 0.5f); }
float3 ClipToViewPos(const float2 clipcoord, float linearDepth) { return float3(clipcoord * float2(pk_MATRIX_I_P[0][0], pk_MATRIX_I_P[1][1]), 1) * linearDepth; }
float3 ClipUVToViewPos(const float2 uv, float linearDepth) { return ClipToViewPos(uv * 2 - 1, linearDepth); }
float3 ClipToViewPos(const float3 clippos) { return ClipToViewPos(clippos.xy, LinearizeDepth(clippos.z)); }
float3 ClipToUVW(const float4 clippos) { return (clippos.xyz / clippos.w) * 0.5f + 0.5f; }
float2 ClipToUV(const float3 clipposxyw) { return (clipposxyw.xy / clipposxyw.z) * 0.5f + 0.5f; }
float4 ClipToScreenPos(const float4 clippos) { return float4(clippos.xy * 0.5f + clippos.w * 0.5f, clippos.zw); }
float3 WorldToClipUVW(const float3 worldpos) { return ClipToUVW(WorldToClipPos(worldpos)); }
float2 ViewToClipUV(const float3 viewpos) { return ClipToUV(mul(pk_MATRIX_P, float4(viewpos, 1.0f)).xyw); }
float3 ScreenToViewPos(const float3 screenpos) { return ClipToViewPos(float3(screenpos.xy * pk_ScreenParams.zw.xy * 2.0f - 1.0f, screenpos.z)); }
float3 ScreenToViewPos(const float2 screenpos, float linearDepth) { return ClipUVToViewPos(screenpos.xy * pk_ScreenParams.zw, linearDepth); }

//----------TESTS----------//
bool Test_DepthReproject(const float z, const float zprev, const float bias) { return (abs(z - zprev - pk_ViewSpaceCameraDelta.z) / z) < bias; }
bool Test_DepthFar(const float depth) { return depth < (pk_ProjectionParams.z - 1e-2f); }
bool Test_ClipPos(const float4 clippos) { return clippos.z > 0.0f && all(lessThan(abs(clippos.xy / clippos.w), 1.0f.xx)); }
bool Test_WorldToClipSpace(float3 worldpos) { return Test_ClipPos(WorldToClipPos(worldpos)); }
bool Test_InScreen(float2 uv) { return All_Equal(saturate( uv ), uv); }
bool Test_InScreen(int2 coord) { return All_InArea(coord, int2(0), int2(pk_ScreenSize.xy)); }

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

#define ReplaceIfResized(v, r) (pk_FrameIndex.y == 0u? r : v)

#endif