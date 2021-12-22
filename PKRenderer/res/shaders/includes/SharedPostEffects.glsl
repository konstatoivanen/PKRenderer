#pragma once
#include Utilities.glsl

PK_DECLARE_CBUFFER(pk_PostEffectsParams, PK_SET_PASS)
{
    float pk_MinLogLuminance;
    float pk_InvLogLuminanceRange;
    float pk_LogLuminanceRange;
    float pk_TargetExposure;
    float pk_AutoExposureSpeed;
    float pk_BloomIntensity;
    float pk_BloomDirtIntensity;
    float pk_Vibrance;
    float4 pk_VignetteGrain;
    float4 pk_WhiteBalance;
	float4 pk_Lift;
	float4 pk_Gamma;
	float4 pk_Gain;
	float4 pk_ContrastGainGammaContribution;
	float4 pk_HSV;
	float4 pk_ChannelMixerRed;
	float4 pk_ChannelMixerGreen;
	float4 pk_ChannelMixerBlue;
};