#pragma once
#include Utilities.glsl

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
};

PK_DECLARE_SET_PASS uniform sampler3D pk_CC_LutTex;