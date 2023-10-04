#pragma once
#ifndef PK_COMMON
#define PK_COMMON

#include Utilities.glsl
#include Constants.glsl

PK_DECLARE_CBUFFER(pk_PerFrameConstants, PK_SET_GLOBAL)
{
    float3x4 pk_MATRIX_V;       // Current view matrix.
    float3x4 pk_MATRIX_I_V;     // Current inverse view matrix.
    float3x4 pk_MATRIX_L_I_V;   // Last inverse view matrix.

    float4x4 pk_MATRIX_P;       // Current projection matrix.
    float4x4 pk_MATRIX_VP;      // Current view * projection matrix.
    float4x4 pk_MATRIX_VP_N;    // Current view * unjittered projection matrix.

    float4x4 pk_MATRIX_L_VP;    // Last view * projection matrix.
    float4x4 pk_MATRIX_L_VP_N;  // Last view * unjittered projection matrix.
    float4x4 pk_MATRIX_L_VP_D;  // Last view * projection * current inverse view matrix.

    float4 pk_Time;      // Time since load (t/20, t, t*2, t*3), use to animate things inside the shaders.
    float4 pk_SinTime;   // Sine of time: (t/8, t/4, t/2, t).
    float4 pk_CosTime;   // Cosine of time: (t/8, t/4, t/2, t).
    float4 pk_DeltaTime; // Delta time: (dt, 1/dt, smoothDt, 1/smoothDt).
    
    float4 pk_CursorParams;         // xy = cursor screen position, zw = cursor screen delta.
    float4 pk_WorldSpaceCameraPos;  // World space position of the camera.
    float4 pk_ViewSpaceCameraDelta; // View space delta position of the camera.
    float4 pk_ProjectionParams;     // x = n, y = f, z = -1 / f, w = -(n - f) / (f * n).
    float4 pk_InvProjectionParams;  // i_p 00, 11, 23, 33
    float4 pk_ExpProjectionParams;  // x = 1.0f / log2(f / n), y = -log2(n) / log2(f / n), z = f / n, w = 1.0f / n.
    float4 pk_ScreenParams;         // xy = current screen (width, height), z = 1 / width, w = 1 / height.
    float4 pk_ShadowCascadeZSplits; // view space z axis splits for directional light shadow cascades
    float4 pk_ProjectionJitter;     // xy = sub pixel jitter, zw = previous frame jitter
    uint4 pk_FrameRandom;           // Random uint4 values for current frame.
    uint2 pk_ScreenSize;            // xy = current screen size
    uint2 pk_FrameIndex;            // x = frame index since load, y = frame index since resize

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

uint GetShadowCascadeIndex(float viewDepth)
{
    uint3 mask = uint3(greaterThan(viewDepth.xxx, pk_ShadowCascadeZSplits.yzw));
    return mask.x + mask.y + mask.z;
}

//----------TRANSFORMS----------//
float3 ObjectToWorldPos(const float3 pos) { return mul(pk_MATRIX_M, float4(pos, 1.0f)).xyz; }
float3 ObjectToWorldDir(const float3 dir) { return mul(float3x3(pk_MATRIX_M), dir); }
float3 ObjectToViewPos(const float3 pos) { return mul(mul(pk_MATRIX_M, float4(pos, 1.0f)), pk_MATRIX_V).xyz; }
float3 ObjectToViewDir(const float3 dir) { return mul(ObjectToWorldDir(dir), float3x3(pk_MATRIX_V)); }
float4 ObjectToClipPos(const float3 pos) { return mul(pk_MATRIX_VP, mul(pk_MATRIX_M, float4(pos, 1.0f))); }

float3 WorldToObjectPos(const float3 pos) { return mul(pk_MATRIX_I_M, float4(pos, 1.0f)).xyz; }
float3 WorldToObjectDir(const float3 dir) { return mul(float3x3(pk_MATRIX_I_M), dir); }
float3 WorldToViewPos(const float3 pos) { return mul(float4(pos, 1.0f), pk_MATRIX_V).xyz; }
float3 WorldToViewDir(const float3 dir) { return mul(dir, float3x3(pk_MATRIX_V)); }
float4 WorldToClipPos(const float3 pos) { return mul(pk_MATRIX_VP, float4(pos, 1.0f)); }
float4 WorldToClipDir(const float3 dir) { return mul(pk_MATRIX_VP, float4(dir, 0.0f)); }
float4 WorldToPrevClipPos(const float3 pos) { return mul(pk_MATRIX_L_VP, float4(pos, 1.0f)); }

float4 ViewToClipPos(const float3 pos) { return mul(pk_MATRIX_P, float4(pos, 1.0f)); }
float4 ViewToClipDir(const float3 dir) { return mul(pk_MATRIX_P, float4(dir, 0.0f)); }
float4 ViewToPrevClipPos(const float3 pos) { return mul(pk_MATRIX_L_VP_D, float4(pos, 1.0f)); }
float3 ViewToWorldPos(const float3 pos) { return mul(float4(pos, 1.0f), pk_MATRIX_I_V).xyz; }
float3 ViewToWorldDir(const float3 dir) { return mul(dir, float3x3(pk_MATRIX_I_V)); }

float  ViewDepth(const float clip_z)      { return 1.0f / (pk_InvProjectionParams.z * clip_z + pk_InvProjectionParams.w); } 
float4 ViewDepth(const float4 clip_z)     { return 1.0f / (pk_InvProjectionParams.z * clip_z + pk_InvProjectionParams.w); } 
float  ClipDepth(const float view_z)      { return fma(1.0f / view_z, pk_ProjectionParams.w, pk_ProjectionParams.z); }
float4 ClipDepth(const float4 view_z)     { return fma(1.0f / view_z, pk_ProjectionParams.wwww, pk_ProjectionParams.zzzz); }
float  ViewDepthExp(const float clip_z)   { return pk_ProjectionParams.x * pow(pk_ExpProjectionParams.z, clip_z); }
float2 ViewDepthExp(const float2 clip_z)  { return pk_ProjectionParams.x * pow(pk_ExpProjectionParams.zz, clip_z); }
float  ClipDepthExp(const float view_z)   { return log2(view_z) * pk_ExpProjectionParams.x + pk_ExpProjectionParams.y; }
float2 ClipDepthExp(const float2 view_z)  { return log2(view_z) * pk_ExpProjectionParams.x + pk_ExpProjectionParams.y; }

float3 UVToViewPos(const float2 uv, float viewDepth) { return float3((uv * 2.0f - 1.0f) * pk_InvProjectionParams.xy, 1.0f) * viewDepth; }
float3 UVToWorldPos(const float2 uv, float viewDepth) { return ViewToWorldPos(UVToViewPos(uv, viewDepth)); }
float3 ClipToUVW(const float4 clip) { return (clip.xyz / clip.w) * float3(0.5f.xx, 1.0f) + float3(0.5f.xx, 0.0f); }
float2 ClipToUV(const float3 clipxyw) { return (clipxyw.xy / clipxyw.z) * 0.5f + 0.5f; }
float3 WorldToClipUVW(const float3 worldpos) { return ClipToUVW(WorldToClipPos(worldpos)); }
float2 ViewToClipUV(const float3 viewpos) { return ClipToUV(ViewToClipPos(viewpos).xyw); }
float2 ViewToPrevClipUV(const float3 viewpos) { return ClipToUV(ViewToPrevClipPos(viewpos).xyw); }
float2 WorldToPrevClipUV(const float3 viewpos) { return ClipToUV(WorldToPrevClipPos(viewpos).xyw); }
// ((z * -n / (f - n)) + 1.0f) / (z * f * n / (f - n));
float2 ClampUVScreenBorder(const float2 uv) { return clamp(uv, 0.5f * pk_ScreenParams.zw, 1.0f.xx - pk_ScreenParams.zw * 0.5f); }
float2 JitterUV(float2 uv) { return uv + pk_ProjectionJitter.xy * 0.5f * pk_ScreenParams.zw; }
float2 DejitterUV(float2 uv) { return uv - pk_ProjectionJitter.xy * 0.5f * pk_ScreenParams.zw; }
float2 JitterPrevUV(float2 uv) { return uv + pk_ProjectionJitter.zw * 0.5f * pk_ScreenParams.zw; }
float2 DejitterPrevUV(float2 uv) { return uv - pk_ProjectionJitter.zw * 0.5f * pk_ScreenParams.zw; }

//----------TESTS----------//
bool4 Test_DepthFar(const float4 depth) { return lessThan(depth, pk_ProjectionParams.yyyy - 1e-2f); }
bool4 Test_DepthReproject(const float4 z, const float4 zprev, const float4 bias) { return lessThan(abs(z - zprev - pk_ViewSpaceCameraDelta.zzzz) / z, bias); }
bool Test_DepthFar(const float depth) { return depth < (pk_ProjectionParams.y - 1e-2f); }
bool Test_DepthReproject(const float z, const float zprev, const float bias) { return Test_DepthFar(zprev) && (abs(z - zprev - pk_ViewSpaceCameraDelta.z) / z) < bias; }
bool Test_ClipPos(const float4 clippos) { return clippos.z > 0.0f && All_Less(abs(clippos.xy / clippos.w), 1.0f.xx); }
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