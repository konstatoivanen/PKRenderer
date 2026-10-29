#pragma once
#ifndef PK_COMMON
#define PK_COMMON

#include "Utilities.glsl"
#include "Constants.glsl"

uniform cbuffer PKRenderView
{
    float3x4 WorldToView;     // Current view matrix.
    float3x4 ViewToWorld;     // Current inverse view matrix.
    float3x4 ViewToWorldPrev; // Last inverse view matrix.

    // Note that projection uses reverse Z
    // This is also in all clip uvw z coordinates.
    float4x4 ViewToClip;             // Current projection matrix.
    float4x4 WorldToClip;            // Current view * projection matrix.
    float4x4 WorldToClip_NoJitter;   // Current view * unjittered projection matrix.

    float4x4 WorldToClipPrev;           // Last view * projection matrix.
    float4x4 WorldToClipPrev_NoJitter;  // Last view * unjittered projection matrix.
    float4x4 ViewToPrevClip;            // Last view * projection * current inverse view matrix.
    float4x4 ClipToPrevClip_NoJitter;   // Last view * projection * current inverse view * projection matrix.

    float4 Time;      // Time since load (t/20, t, t*2, t*3), use to animate things inside the shaders.
    float4 SinTime;   // Sine of time: (t/8, t/4, t/2, t).
    float4 CosTime;   // Cosine of time: (t/8, t/4, t/2, t).
    float4 DeltaTime; // Delta time: (dt, 1/dt, smoothDt, 1/smoothDt).
    
    float4 CursorParams;         // xy = cursor screen position, zw = cursor screen delta.
    float4 ViewWorldOrigin;      // World space position of the view frustum
    float4 ViewWorldOriginPrev;  // Previous World space position of the camera.
    float4 ViewSpaceCameraDelta; // View space delta position of the camera.
    float4 ClipParams;           // x = n, y = f, z = -1 / f, w = -(n - f) / (f * n).
    float4 ClipParamsInv;        // i_p 00, 11, 23, 33
    float4 ScreenParams;         // xy = current screen (width, height), z = 1 / width, w = 1 / height.
    float4 ProjectionJitter;     // xy = sub pixel jitter, zw = previous frame jitter
    uint4 FrameRandom;           // Random uint4 values for current frame.
    uint2 ScreenSize;            // xy = current screen size
    uint2 FrameIndex;            // x = frame index since load, y = frame index since resize
    
    float4 MeshletCullParams;    // Meshlet Error Scale Factor, Horizontal Fov, Vertical Fov, Unused.
    float4 ShadowCascadeZSplits; // view space z axis splits for directional light shadow cascades
    float4 LightTileZParams;     // exponential depth conversion params for light grid z coordinates

    int ScreenLevels; // Maximum mip level of current screen dimensions.
    
    // GI Parameters
    float3 GI_VX_TexelSize;
    float GI_VX_StepSize;
    uint3 GI_VX_Swizzle;
    float GI_VX_LevelScale;
    float4 GI_VX_ST;
    uint2 GI_RayDither;

    // Fog Parameters
    float Fog_Density_NoiseAmount;
    float Fog_Density_NoiseScale;
    float Fog_Density_Amount;
    float Fog_Phase0;
    float Fog_Phase1;
    float Fog_PhaseW;
    float4 Fog_Albedo;
    float4 Fog_ZParams;
    float4 Fog_FadeParams;
    float4 Fog_Absorption;
    float4 Fog_WindDirSpeed;
    float4 Fog_Density_ExpParams0;
    float4 Fog_Density_ExpParams1;

    float4 Panini_Projection_Parameters;

    // Post FX Parameters
    float4 CC_WhiteBalance;
    float4 CC_Lift;
    float4 CC_Gamma;
    float4 CC_Gain;
    float4 CC_HSV;
    float4 CC_MixRed;
    float4 CC_MixGreen;
    float4 CC_MixBlue;
    float CC_LumaContrast;
    float CC_LumaGain;
    float CC_LumaGamma;
    float CC_Vibrance;
    float CC_Contribution;

    float Vignette_Intensity;
    float Vignette_Power;

    float FilmGrain_Luminance;
    float FilmGrain_Intensity;
    float FilmGrain_ExposureSensitivity;

    float AutoExposure_LogLumaRange;
    float AutoExposure_Target;
    float AutoExposure_Min;
    float AutoExposure_Max;
    float AutoExposure_Speed;

    float Bloom_Diffusion;
    float Bloom_Intensity;
    float Bloom_DirtIntensity;

    float Chromatic_Aberration_Amount;
    float Chromatic_Aberration_Power;

    float TAA_Sharpness;
    float TAA_BlendingStatic;
    float TAA_BlendingMotion;
    float TAA_MotionAmplification;

    uint PostEffectsFeatureMask;
} pk_View;

#if !defined(PK_INSTANCING_ENABLED)
uniform cbuffer pk_ModelMatrices
{
    float3x4 pk_ObjectToWorld; // Current model matrix.
};
#endif

uniform sampler pk_SamplerBilinearRepeat;
uniform sampler pk_SamplerBilinearClamped;
uniform sampler pk_SamplerTrilinearBorder;
uniform sampler pk_SamplerPointClamped;
uniform sampler pk_SamplerTrilinearRepeatAniso;
uniform accelerationStructureEXT pk_SceneStructure;

uint GetShadowCascadeIndex(float view_depth)
{
    uint3 mask = uint3(greaterThan(view_depth.xxx, pk_View.ShadowCascadeZSplits.xyz));
    return mask.x + mask.y + mask.z;
}

//----------TRANSFORMS----------//
float  ViewDepth(const float clip_z)      { return 1.0f / (pk_View.ClipParamsInv.z * clip_z + pk_View.ClipParamsInv.w); } 
float4 ViewDepth(const float4 clip_z)     { return 1.0f / (pk_View.ClipParamsInv.z * clip_z + pk_View.ClipParamsInv.w); } 
float  ClipDepth(const float view_z)      { return fma(1.0f / view_z, pk_View.ClipParams.w, pk_View.ClipParams.z); }
float4 ClipDepth(const float4 view_z)     { return fma(1.0f / view_z, pk_View.ClipParams.wwww, pk_View.ClipParams.zzzz); }

// Note that these dont produce reverse Z as theyre used for exponential mapping of volumes where 0-N range is preferred.
float  ViewDepthExp(const float clip_z, const float3 params) { return (exp2(clip_z / params.z) - params.y) / params.x; }
float2 ViewDepthExp(const float2 clip_z, const float3 params) { return (exp2(clip_z / params.zz) - params.yy) / params.xx; }
float  ClipDepthExp(const float view_z, const float3 params) { return log2(view_z * params.x + params.y) * params.z; }
float2 ClipDepthExp(const float2 view_z, const float3 params) { return log2(view_z * params.xx + params.yy) * params.zz; }

float3 ClipToUvw(const float4 clip) { return fma(clip.xyz / clip.w, float3(0.5f.xx, 1.0f), float3(0.5f.xx, 0.0f)); }
float2 ClipToUv(const float3 clip_xyw) { return fma(clip_xyw.xy / clip_xyw.z, 0.5f.xx, 0.5f.xx); }

float4 ViewToClipPos(const float3 pos) { return pk_View.ViewToClip * float4(pos, 1.0f); }
float4 ViewToClipPosPrev(const float3 pos) { return pk_View.ViewToPrevClip * float4(pos, 1.0f); }
float4 ViewToClipVec(const float3 vec) { return pk_View.ViewToClip * float4(vec, 0.0f); }
float3 ViewToWorldPos(const float3 pos) { return float4(pos, 1.0f) * pk_View.ViewToWorld; }
float3 ViewToWorldPosPrev(const float3 pos) { return float4(pos, 1.0f) * pk_View.ViewToWorldPrev; }
float3 ViewToWorldVec(const float3 vec) { return vec * float3x3(pk_View.ViewToWorld); }
float2 ViewToClipUv(const float3 pos) { return ClipToUv(ViewToClipPos(pos).xyw); }
float2 ViewToClipUvPrev(const float3 pos) { return ClipToUv(ViewToClipPosPrev(pos).xyw); }
float3 ViewToClipUvw(const float3 pos) { return ClipToUvw(ViewToClipPos(pos)); }
float3 ViewToClipUvwPrev(const float3 pos) { return ClipToUvw(ViewToClipPosPrev(pos)); }

float3 WorldToViewPos(const float3 pos) { return float4(pos, 1.0f) * pk_View.WorldToView; }
float3 WorldToViewVec(const float3 vec) { return vec * float3x3(pk_View.WorldToView); }
float4 WorldToClipPos(const float3 pos) { return pk_View.WorldToClip * float4(pos, 1.0f); }
float4 WorldToClipPosPrev(const float3 pos) { return pk_View.WorldToClipPrev * float4(pos, 1.0f); }
float4 WorldToClipVec(const float3 vec) { return pk_View.WorldToClip * float4(vec, 0.0f); }
float3 WorldToClipUvw(const float3 pos) { return ClipToUvw(WorldToClipPos(pos)); }
float2 WorldToClipUvPrev(const float3 pos) { return ClipToUv(WorldToClipPosPrev(pos).xyw); }
float3 WorldToClipUvwPrev(const float3 pos) { return ClipToUvw(WorldToClipPosPrev(pos)); }

float3 ObjectToWorldPos(const float3 pos) { return float4(pos, 1.0f) * pk_ObjectToWorld; }
float3 ObjectToWorldVec(const float3 vec) { return vec * float3x3(pk_ObjectToWorld); }
float3 ObjectToViewPos(const float3 pos) { return WorldToViewPos(ObjectToWorldPos(pos)); }
float3 ObjectToViewVec(const float3 vec) { return WorldToViewVec(ObjectToWorldVec(vec)); }
float4 ObjectToClipPos(const float3 pos) { return pk_View.WorldToClip * float4(ObjectToWorldPos(pos), 1.0f); }

float3 UvToViewPos(const float2 uv, float view_depth) { return float3((uv * 2.0f - 1.0f) * pk_View.ClipParamsInv.xy, 1.0f) * view_depth; }
float3 UvToViewDir(const float2 uv) { return normalize(UvToViewPos(uv, 1.0f)); }
float3 UvToWorldPos(const float2 uv, float view_depth) { return ViewToWorldPos(UvToViewPos(uv, view_depth)); }

float3 CoordToViewPos(const int2 coord, const float view_depth) { return UvToViewPos((coord + 0.5f.xx) * pk_View.ScreenParams.zw, view_depth); }
float3 CoordToWorldPos(const int2 coord, const float view_depth) { return ViewToWorldPos(CoordToViewPos(coord, view_depth)); }
float3 CoordToWorldPosPrev(const int2 coord, const float view_depth) { return ViewToWorldPosPrev(CoordToViewPos(coord, view_depth)); }

float2 ClampBilinearViewUv(const float2 uv) { return clamp(uv, 0.5f * pk_View.ScreenParams.zw, 1.0f.xx - pk_View.ScreenParams.zw * 0.5f); }

//----------TESTS----------//
bool4 Test_DepthIsScene(const float4 depth) { return lessThan(depth, pk_View.ClipParams.yyyy - 1e-2f); }
bool4 Test_DepthReproject(const float4 z, const float4 zprev, const float4 bias) { return lessThan(abs(z - zprev - pk_View.ViewSpaceCameraDelta.zzzz) / z, bias); }
bool Test_DepthIsScene(const float depth) { return depth < (pk_View.ClipParams.y - 1e-2f); }
bool Test_DepthReproject(const float z, const float zprev, const float bias) { return Test_DepthIsScene(zprev) && (abs(z - zprev - pk_View.ViewSpaceCameraDelta.z) / z) < bias; }
bool Test_DepthSurface(const float z0, const float z1, const float bias) { return Test_DepthIsScene(z1) && (abs(z0 - z1 ) / z0) < bias; }
bool Test_InUv(float2 uv) { return All_Equal(saturate( uv ), uv); }
bool Test_InUvw(float3 uv) { return All_Equal(saturate( uv ), uv); }
bool Test_InScreen(int2 coord) { return All_InArea(coord, int2(0), int2(pk_View.ScreenSize.xy)); }
#define ReplaceIfResized(v, r) (pk_View.FrameIndex.y == 0u? r : v)

#endif
