#pragma once
#ifndef PK_COMMON
#define PK_COMMON

#include "Utilities.glsl"
#include "Constants.glsl"

PK_DECLARE_CBUFFER(pk_PerFrameConstants, PK_SET_GLOBAL)
{
    float3x4 pk_WorldToView;     // Current view matrix.
    float3x4 pk_ViewToWorld;     // Current inverse view matrix.
    float3x4 pk_ViewToWorldPrev; // Last inverse view matrix.

    // Note that projection uses reverse Z
    // This is also in all clip uvw z coordinates.
    float4x4 pk_ViewToClip;             // Current projection matrix.
    float4x4 pk_WorldToClip;            // Current view * projection matrix.
    float4x4 pk_WorldToClip_NoJitter;   // Current view * unjittered projection matrix.

    float4x4 pk_WorldToClipPrev;           // Last view * projection matrix.
    float4x4 pk_WorldToClipPrev_NoJitter;  // Last view * unjittered projection matrix.
    float4x4 pk_ViewToPrevClip;            // Last view * projection * current inverse view matrix.
    float4x4 pk_ClipToPrevClip_NoJitter;   // Last view * projection * current inverse view * projection matrix.

    float4 pk_Time;      // Time since load (t/20, t, t*2, t*3), use to animate things inside the shaders.
    float4 pk_SinTime;   // Sine of time: (t/8, t/4, t/2, t).
    float4 pk_CosTime;   // Cosine of time: (t/8, t/4, t/2, t).
    float4 pk_DeltaTime; // Delta time: (dt, 1/dt, smoothDt, 1/smoothDt).
    
    float4 pk_CursorParams;         // xy = cursor screen position, zw = cursor screen delta.
    float4 pk_ViewWorldOrigin;      // World space position of the view frustum
    float4 pk_ViewWorldOriginPrev;  // Previous World space position of the camera.
    float4 pk_ViewSpaceCameraDelta; // View space delta position of the camera.
    float4 pk_ClipParams;           // x = n, y = f, z = -1 / f, w = -(n - f) / (f * n).
    float4 pk_ClipParamsInv;        // i_p 00, 11, 23, 33
    float4 pk_ScreenParams;         // xy = current screen (width, height), z = 1 / width, w = 1 / height.
    float4 pk_ProjectionJitter;     // xy = sub pixel jitter, zw = previous frame jitter
    uint4 pk_FrameRandom;           // Random uint4 values for current frame.
    uint2 pk_ScreenSize;            // xy = current screen size
    uint2 pk_FrameIndex;            // x = frame index since load, y = frame index since resize
    
    float4 pk_MeshletCullParams;    // Meshlet Error Scale Factor, Horizontal Fov, Vertical Fov, Unused.
    float4 pk_ShadowCascadeZSplits; // view space z axis splits for directional light shadow cascades
    float4 pk_LightTileZParams;     // exponential depth conversion params for light grid z coordinates

    // @TODO redudant in here. remove. Currenty here as it provides padding :)
    float pk_SceneEnv_Exposure; // Scene background environment exposure
    
    // GI Parameters
    float pk_GI_VoxelSize;
    float pk_GI_VoxelStepSize;
    float pk_GI_VoxelLevelScale;
    float4 pk_GI_VolumeST;
    uint4 pk_GI_VolumeSwizzle;
    uint2 pk_GI_RayDither;

    // Fog Parameters
    float pk_Fog_Density_Amount;
    float pk_Fog_Density_Constant;
    float4 pk_Fog_Albedo;
    float4 pk_Fog_ZParams;
    float4 pk_Fog_Absorption;
    float4 pk_Fog_WindDirSpeed;

    float pk_Fog_Phase0;
    float pk_Fog_Phase1;
    float pk_Fog_PhaseW;
    float pk_Fog_Density_HeightExponent;
    
    float pk_Fog_Density_HeightOffset;
    float pk_Fog_Density_HeightAmount;
    float pk_Fog_Density_NoiseAmount;
    float pk_Fog_Density_NoiseScale;
    
    float pk_Fog_Density_Sky_Constant;
    float pk_Fog_Density_Sky_HeightExponent;
    float pk_Fog_Density_Sky_HeightOffset;
    float pk_Fog_Density_Sky_HeightAmount;

    // Post FX Parameters
    float4 pk_CC_WhiteBalance;
    float4 pk_CC_Lift;
    float4 pk_CC_Gamma;
    float4 pk_CC_Gain;
    float4 pk_CC_HSV;
    float4 pk_CC_MixRed;
    float4 pk_CC_MixGreen;
    float4 pk_CC_MixBlue;
    float pk_CC_LumaContrast;
    float pk_CC_LumaGain;
    float pk_CC_LumaGamma;
    float pk_CC_Vibrance;
    float pk_CC_Contribution;

    float pk_Vignette_Intensity;
    float pk_Vignette_Power;

    float pk_FilmGrain_Luminance;
    float pk_FilmGrain_Intensity;
    float pk_FilmGrain_ExposureSensitivity;

    float pk_AutoExposure_LogLumaRange;
    float pk_AutoExposure_Target;
    float pk_AutoExposure_Min;
    float pk_AutoExposure_Max;
    float pk_AutoExposure_Speed;

    float pk_Bloom_Diffusion;
    float pk_Bloom_Intensity;
    float pk_Bloom_DirtIntensity;

    float pk_TAA_Sharpness;
    float pk_TAA_BlendingStatic;
    float pk_TAA_BlendingMotion;
    float pk_TAA_MotionAmplification;

    uint pk_PostEffectsFeatureMask;
};

#if !defined(PK_INSTANCING_ENABLED)
PK_DECLARE_CBUFFER(pk_ModelMatrices, PK_SET_DRAW)
{
    float3x4 pk_ObjectToWorld; // Current model matrix.
};
#endif

PK_DECLARE_ACCELERATION_STRUCTURE(PK_SET_SHADER, pk_SceneStructure)

PK_DECLARE_SET_GLOBAL uniform sampler pk_Sampler_GBuffer;
PK_DECLARE_SET_GLOBAL uniform sampler pk_Sampler_GUI;
PK_DECLARE_SET_PASS uniform sampler pk_Sampler_SurfDefault;

uint GetShadowCascadeIndex(float view_depth)
{
    uint3 mask = uint3(greaterThan(view_depth.xxx, pk_ShadowCascadeZSplits.xyz));
    return mask.x + mask.y + mask.z;
}

//----------TRANSFORMS----------//
float  ViewDepth(const float clip_z)      { return 1.0f / (pk_ClipParamsInv.z * clip_z + pk_ClipParamsInv.w); } 
float4 ViewDepth(const float4 clip_z)     { return 1.0f / (pk_ClipParamsInv.z * clip_z + pk_ClipParamsInv.w); } 
float  ClipDepth(const float view_z)      { return fma(1.0f / view_z, pk_ClipParams.w, pk_ClipParams.z); }
float4 ClipDepth(const float4 view_z)     { return fma(1.0f / view_z, pk_ClipParams.wwww, pk_ClipParams.zzzz); }

// Note that these dont produce reverse Z as theyre used for exponential mapping of volumes where 0-N range is preferred.
float  ViewDepthExp(const float clip_z, const float3 params) { return (exp2(clip_z / params.z) - params.y) / params.x; }
float2 ViewDepthExp(const float2 clip_z, const float3 params) { return (exp2(clip_z / params.zz) - params.yy) / params.xx; }
float  ClipDepthExp(const float view_z, const float3 params) { return log2(view_z * params.x + params.y) * params.z; }
float2 ClipDepthExp(const float2 view_z, const float3 params) { return log2(view_z * params.xx + params.yy) * params.zz; }

float3 ClipToUvw(const float4 clip) { return fma(clip.xyz / clip.w, float3(0.5f.xx, 1.0f), float3(0.5f.xx, 0.0f)); }
float2 ClipToUv(const float3 clip_xyw) { return fma(clip_xyw.xy / clip_xyw.z, 0.5f.xx, 0.5f.xx); }

float4 ViewToClipPos(const float3 pos) { return pk_ViewToClip * float4(pos, 1.0f); }
float4 ViewToClipPosPrev(const float3 pos) { return pk_ViewToPrevClip * float4(pos, 1.0f); }
float4 ViewToClipVec(const float3 vec) { return pk_ViewToClip * float4(vec, 0.0f); }
float3 ViewToWorldPos(const float3 pos) { return float4(pos, 1.0f) * pk_ViewToWorld; }
float3 ViewToWorldPosPrev(const float3 pos) { return float4(pos, 1.0f) * pk_ViewToWorldPrev; }
float3 ViewToWorldVec(const float3 vec) { return vec * float3x3(pk_ViewToWorld); }
float2 ViewToClipUv(const float3 pos) { return ClipToUv(ViewToClipPos(pos).xyw); }
float2 ViewToClipUvPrev(const float3 pos) { return ClipToUv(ViewToClipPosPrev(pos).xyw); }
float3 ViewToClipUvw(const float3 pos) { return ClipToUvw(ViewToClipPos(pos)); }
float3 ViewToClipUvwPrev(const float3 pos) { return ClipToUvw(ViewToClipPosPrev(pos)); }

float3 WorldToViewPos(const float3 pos) { return float4(pos, 1.0f) * pk_WorldToView; }
float3 WorldToViewVec(const float3 vec) { return vec * float3x3(pk_WorldToView); }
float4 WorldToClipPos(const float3 pos) { return pk_WorldToClip * float4(pos, 1.0f); }
float4 WorldToClipPosPrev(const float3 pos) { return pk_WorldToClipPrev * float4(pos, 1.0f); }
float4 WorldToClipVec(const float3 vec) { return pk_WorldToClip * float4(vec, 0.0f); }
float3 WorldToClipUvw(const float3 pos) { return ClipToUvw(WorldToClipPos(pos)); }
float2 WorldToClipUvPrev(const float3 pos) { return ClipToUv(WorldToClipPosPrev(pos).xyw); }
float3 WorldToClipUvwPrev(const float3 pos) { return ClipToUvw(WorldToClipPosPrev(pos)); }

float3 ObjectToWorldPos(const float3 pos) { return float4(pos, 1.0f) * pk_ObjectToWorld; }
float3 ObjectToWorldVec(const float3 vec) { return vec * float3x3(pk_ObjectToWorld); }
float3 ObjectToViewPos(const float3 pos) { return WorldToViewPos(ObjectToWorldPos(pos)); }
float3 ObjectToViewVec(const float3 vec) { return WorldToViewVec(ObjectToWorldVec(vec)); }
float4 ObjectToClipPos(const float3 pos) { return pk_WorldToClip * float4(ObjectToWorldPos(pos), 1.0f); }

float3 UvToViewPos(const float2 uv, float view_depth) { return float3((uv * 2.0f - 1.0f) * pk_ClipParamsInv.xy, 1.0f) * view_depth; }
float3 UvToViewDir(const float2 uv) { return normalize(UvToViewPos(uv, 1.0f)); }
float3 UvToWorldPos(const float2 uv, float view_depth) { return ViewToWorldPos(UvToViewPos(uv, view_depth)); }

float3 CoordToViewPos(const int2 coord, const float view_depth) { return UvToViewPos((coord + 0.5f.xx) * pk_ScreenParams.zw, view_depth); }
float3 CoordToWorldPos(const int2 coord, const float view_depth) { return ViewToWorldPos(CoordToViewPos(coord, view_depth)); }
float3 CoordToWorldPosPrev(const int2 coord, const float view_depth) { return ViewToWorldPosPrev(CoordToViewPos(coord, view_depth)); }

float2 ClampBilinearViewUv(const float2 uv) { return clamp(uv, 0.5f * pk_ScreenParams.zw, 1.0f.xx - pk_ScreenParams.zw * 0.5f); }

//----------TESTS----------//
bool4 Test_DepthIsScene(const float4 depth) { return lessThan(depth, pk_ClipParams.yyyy - 1e-2f); }
bool4 Test_DepthReproject(const float4 z, const float4 zprev, const float4 bias) { return lessThan(abs(z - zprev - pk_ViewSpaceCameraDelta.zzzz) / z, bias); }
bool Test_DepthIsScene(const float depth) { return depth < (pk_ClipParams.y - 1e-2f); }
bool Test_DepthReproject(const float z, const float zprev, const float bias) { return Test_DepthIsScene(zprev) && (abs(z - zprev - pk_ViewSpaceCameraDelta.z) / z) < bias; }
bool Test_DepthSurface(const float z0, const float z1, const float bias) { return Test_DepthIsScene(z1) && (abs(z0 - z1 ) / z0) < bias; }
bool Test_InUv(float2 uv) { return All_Equal(saturate( uv ), uv); }
bool Test_InUvw(float3 uv) { return All_Equal(saturate( uv ), uv); }
bool Test_InScreen(int2 coord) { return All_InArea(coord, int2(0), int2(pk_ScreenSize.xy)); }
#define ReplaceIfResized(v, r) (pk_FrameIndex.y == 0u? r : v)

#endif
