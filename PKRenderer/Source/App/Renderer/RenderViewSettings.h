#pragma once
#include "Core/Math/Math.h"
#include "Core/Yaml/ConvertMathTypes.h"
#include "Core/Yaml/ConfigMacros.h"
#include "Core/Yaml/ConvertCVariableCollection.h"
#include "Core/Yaml/ConvertTextureAsset.h"

namespace PK::App
{
    PK_YAML_STRUCT_BEGIN(PostEffectsSettings)
        PK_YAML_MEMBER(bool, Vignette, true)
        PK_YAML_MEMBER(bool, Bloom, true)
        PK_YAML_MEMBER(bool, Tonemap, true)
        PK_YAML_MEMBER(bool, Filmgrain, true)
        PK_YAML_MEMBER(bool, Colorgrading, false)
        PK_YAML_MEMBER(bool, LUTColorGrading, true)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(RenderingDebugSettings)
        PK_YAML_MEMBER(bool, GIDiff, false)
        PK_YAML_MEMBER(bool, GISpec, false)
        PK_YAML_MEMBER(bool, GIVX, false)
        PK_YAML_MEMBER(bool, Normal, false)
        PK_YAML_MEMBER(bool, Roughness, false)
        PK_YAML_MEMBER(bool, HalfScreen, false)
        PK_YAML_MEMBER(bool, Zoom, false)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(DepthOfFieldSettings)
        PK_YAML_MEMBER(float, FocalLength, 0.05f)
        PK_YAML_MEMBER(float, FNumber, 1.40f)
        PK_YAML_MEMBER(float, FilmHeight, 0.024f)
        PK_YAML_MEMBER(float, FocusSpeed, 5.0f)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(AutoExposureSettings)
        PK_YAML_MEMBER(float, LuminanceMin, 1.0f)
        PK_YAML_MEMBER(float, LuminanceRange, 1.0f)
        PK_YAML_MEMBER(float, ExposureSpeed, 1.0f)
        PK_YAML_MEMBER(float, ExposureTarget, 1.0f)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(FilmGrainSettings)
        PK_YAML_MEMBER(float, Intensity, 0.25f)
        PK_YAML_MEMBER(float, Luminance, 0.25f)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(VignetteSettings)
        PK_YAML_MEMBER(float, Intensity, 15.0f)
        PK_YAML_MEMBER(float, Power, 0.25f)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(ColorGradingSettings)
        PK_YAML_MEMBER(float, Contribution, 1.0f)
        PK_YAML_MEMBER(float, TemperatureShift, 0.0f)
        PK_YAML_MEMBER(float, Tint, 0.0f)
        PK_YAML_MEMBER(float, Hue, 0.0f)
        PK_YAML_MEMBER(float, Saturation, 1.0f)
        PK_YAML_MEMBER(float, Vibrance, 0.0f)
        PK_YAML_MEMBER(float, Value, 1.0f)
        PK_YAML_MEMBER(float, Contrast, 1.0f)
        PK_YAML_MEMBER(float, Gain, 1.0f)
        PK_YAML_MEMBER(float, Gamma, 1.0f)
        PK_YAML_MEMBER(uint, Shadows, 0x000000FF)
        PK_YAML_MEMBER(uint, Midtones, 0x7F7F7FFF)
        PK_YAML_MEMBER(uint, Highlights, 0xFFFFFFFF)
        PK_YAML_MEMBER(uint, ChannelMixerRed, 0xFF0000FF)
        PK_YAML_MEMBER(uint, ChannelMixerGreen, 0x00FF00FF)
        PK_YAML_MEMBER(uint, ChannelMixerBlue, 0x0000FFFF)
        PK_YAML_MEMBER(TextureAsset*, LutTextureAsset, nullptr)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(BloomSettings)
        PK_YAML_MEMBER(float, Intensity, 0.0f)
        PK_YAML_MEMBER(float, LensDirtIntensity, 0.0f)
        PK_YAML_MEMBER(TextureAsset*, LensDirtTextureAsset, nullptr)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(TemporalAntialiasingSettings)
        PK_YAML_MEMBER(float, Sharpness, 0.5f)
        PK_YAML_MEMBER(float, BlendingStatic, 0.99f)
        PK_YAML_MEMBER(float, BlendingMotion, 0.85f)
        PK_YAML_MEMBER(float, MotionAmplification, 600.0f)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(FogSettings)
        PK_YAML_MEMBER(float3, Albedo, PK_FLOAT3_ONE)
        PK_YAML_MEMBER(float3, Absorption, PK_FLOAT3_ONE)
        PK_YAML_MEMBER(float, Phase0, 0.25f)
        PK_YAML_MEMBER(float, Phase1, 0.95f)
        PK_YAML_MEMBER(float, PhaseW, 0.5f)
        PK_YAML_MEMBER(float3, WindDirection, PK_FLOAT3_FORWARD)
        PK_YAML_MEMBER(float, WindSpeed, 0.0f)
        PK_YAML_MEMBER(float, DensityConstant, 0.0f)
        PK_YAML_MEMBER(float, DensityHeightExponent, 0.0f)
        PK_YAML_MEMBER(float, DensityHeightOffset, 0.0f)
        PK_YAML_MEMBER(float, DensityHeightAmount, 0.0f)
        PK_YAML_MEMBER(float, DensityNoiseAmount, 0.0f)
        PK_YAML_MEMBER(float, DensityNoiseScale, 0.0f)
        PK_YAML_MEMBER(float, Density, 0.0f)
        PK_YAML_MEMBER(float, DensitySkyConstant, 0.0f)
        PK_YAML_MEMBER(float, DensitySkyHeightExponent, 0.0f)
        PK_YAML_MEMBER(float, DensitySkyHeightOffset, 0.0f)
        PK_YAML_MEMBER(float, DensitySkyHeightAmount, 0.0f)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(EnvBackgroundSettings)
        PK_YAML_MEMBER(float, Exposure, 1.0f)
        PK_YAML_MEMBER(TextureAsset*, EnvironmentTextureAsset, nullptr)
    PK_YAML_STRUCT_END()

    PK_YAML_STRUCT_BEGIN(RenderViewSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(PostEffectsSettings, PostEffectSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(RenderingDebugSettings, RenderingDebugSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(DepthOfFieldSettings, DepthOfFieldSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(AutoExposureSettings, AutoExposureSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(FilmGrainSettings, FilmGrainSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(VignetteSettings, VignetteSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(ColorGradingSettings, ColorGradingSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(BloomSettings, BloomSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(TemporalAntialiasingSettings, TemporalAntialiasingSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(FogSettings, FogSettings)
        PK_YAML_MEMBER_INLINE_STRUCT(EnvBackgroundSettings, EnvBackgroundSettings)
    PK_YAML_STRUCT_END()
}