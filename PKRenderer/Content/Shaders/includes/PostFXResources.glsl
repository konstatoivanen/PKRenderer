#pragma once
#include "Utilities.glsl"

PK_DECLARE_CBUFFER(pk_PostEffectsParams, PK_SET_PASS)
{
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

    float pk_AutoExposure_MinLogLuma;
    float pk_AutoExposure_InvLogLumaRange;
    float pk_AutoExposure_LogLumaRange;
    float pk_AutoExposure_Target;
    float pk_AutoExposure_Speed;

    float pk_Bloom_Intensity;
    float pk_Bloom_DirtIntensity;

    float pk_TAA_Sharpness;
    float pk_TAA_BlendingStatic;
    float pk_TAA_BlendingMotion;
    float pk_TAA_MotionAmplification;

    uint pk_PostEffectsFeatureMask;
};

PK_DECLARE_SET_PASS uniform sampler3D pk_CC_LutTex;

#define FX_FEAT_VIGNETTE 0x1u
#define FX_FEAT_BLOOM 0x2u
#define FX_FEAT_TONEMAP 0x4u
#define FX_FEAT_FILMGRAIN 0x8u
#define FX_FEAT_COLORGRADING 0x10u
#define FX_FEAT_LUTCOLORGRADING 0x20u

#define FX_FEAT_DEBUG_GI_DIFF 0x40u
#define FX_FEAT_DEBUG_GI_SPEC 0x80u
#define FX_FEAT_DEBUG_GI_VX 0x100u
#define FX_FEAT_DEBUG_NORMAL 0x200u
#define FX_FEAT_DEBUG_ROUGHNESS 0x400u
#define FX_FEAT_DEBUG_HALFSCREEN 0x800u
#define FX_FEAT_DEBUG_ZOOM 0x1000u

#if defined(FX_APPLY_ALL)
#define IF_FX_FEATURE_ENABLED(feature)
#define IS_FX_FEATURE_ENABLED(feature)
#else
#define IF_FX_FEATURE_ENABLED(feature) if ((pk_PostEffectsFeatureMask & feature) != 0u)
#define IS_FX_FEATURE_ENABLED(feature) ((pk_PostEffectsFeatureMask & feature) != 0u)
#endif