#pragma once
#include "Core/Math/Math.h"

namespace PK
{
    struct TextureAsset;
}

namespace PK::App
{
    struct PostEffectsSettings
    {
        bool Vignette = true;
        bool Bloom = true;
        bool Tonemap = true;
        bool Filmgrain = true;
        bool Colorgrading = false;
        bool LUTColorGrading = true;
    };

    struct RenderingDebugSettings
    {
        bool GIDiff = false;
        bool GISpec = false;
        bool GIVX = false;
        bool LightTiles = false;
        bool Normal = false;
        bool Roughness = false;
        bool HalfScreen = false;
        bool Zoom = false;
    };

    struct DepthOfFieldSettings
    {
        float FocalLength = 0.05f;
        float FNumber = 1.40f;
        float FilmHeight = 0.024f;
        float FocusSpeed = 5.0f;
    };

    struct AutoExposureSettings
    {
        float LogLuminanceRange = 1.0f;
        float ExposureMin = 0.01f;
        float ExposureMax = 1.0f;
        float ExposureTarget = 1.0f;
        float ExposureSpeed = 1.0f;
    };

    struct FilmGrainSettings
    {
        float Intensity = 0.25f;
        float Luminance = 0.25f;
        float ExposureSensitivity = 1.0f;
    };

    struct VignetteSettings
    {
        float Intensity = 15.0f;
        float Power = 0.25f;
    };

    struct DistortSettings
    {
        float PaniniProjectionAmount = 0.0f;
        float PaniniProjectionScreenFit = 1.0f;
        float ChromaticAberrationAmount = 0.0f;
        float ChromaticAberrationPower = 1.0f;
    };

    struct ColorGradingSettings
    {
        float Contribution = 1.0f;
        float TemperatureShift = 0.0f;
        float Tint = 0.0f;
        float Hue = 0.0f;
        float Saturation = 1.0f;
        float Vibrance = 0.0f;
        float Value = 1.0f;
        float Contrast = 1.0f;
        float Gain = 1.0f;
        float Gamma = 1.0f;
        uint Shadows = 0x000000FF;
        uint Midtones = 0x7F7F7FFF;
        uint Highlights = 0xFFFFFFFF;
        uint ChannelMixerRed = 0xFF0000FF;
        uint ChannelMixerGreen = 0x00FF00FF;
        uint ChannelMixerBlue = 0x0000FFFF;
        TextureAsset* LutTextureAsset = nullptr;
        TextureAsset* TonemapLutTextureAsset = nullptr;
    };

    struct BloomSettings
    {
        float Intensity = 0.0f;
        float Diffusion = 1.0f;
        bool  BorderClip = true;
        float LensDirtIntensity = 0.0f;
        TextureAsset* LensDirtTextureAsset = nullptr;
    };

    struct TemporalAntialiasingSettings
    {
        float Sharpness = 0.5f;
        float BlendingStatic = 0.99f;
        float BlendingMotion = 0.85f;
        float MotionAmplification = 600.0f;
    };

    struct FogExponentialSettings
    {
        float Constant = 0.0f;
        float HeightExponent = 0.0f;
        float HeightOffset = 0.0f;
        float HeightAmount = 0.0f;
    };

    struct FogSettings
    {
        float ZNear = 0.2f;
        float ZFar = 200.0f;
        float ZDistribution = 0.185f;
        float FadeShadowsDirect = 0.05f;
        float FadeShadowsVolumetric = 0.25f;
        float FadeStatic = 0.25f;
        float FadeGroundOcclusion = 3.0f;
        float3 Albedo = PK_FLOAT3_ONE;
        float3 Absorption = PK_FLOAT3_ONE;
        float Phase0 = 0.25f;
        float Phase1 = 0.95f;
        float PhaseW = 0.5f;
        float3 WindDirection = PK_FLOAT3_FORWARD;
        float WindSpeed = 0.0f;
        float Density = 0.0f;
        float DensityNoiseAmount = 0.0f;
        float DensityNoiseScale = 0.0f;
        FogExponentialSettings Exponential0;
        FogExponentialSettings Exponential1;
    };

    struct EnvBackgroundSettings
    {
        float Exposure = 1.0f;
        bool CaptureUsesViewOrigin = false;
        int32_t CaptureInterval = -1;
        float3 CaptureOffset = PK_FLOAT3_ZERO;
        TextureAsset* EnvironmentTextureAsset = nullptr;
    };

    struct RenderViewSettings
    {
        PostEffectsSettings PostEffectSettings;
        RenderingDebugSettings RenderingDebugSettings;
        DepthOfFieldSettings DepthOfFieldSettings;
        AutoExposureSettings AutoExposureSettings;
        FilmGrainSettings FilmGrainSettings;
        VignetteSettings VignetteSettings;
        DistortSettings DistortSettings;
        ColorGradingSettings ColorGradingSettings;
        BloomSettings BloomSettings;
        TemporalAntialiasingSettings TemporalAntialiasingSettings;
        FogSettings FogSettings;
        EnvBackgroundSettings EnvBackgroundSettings;
    };
}
