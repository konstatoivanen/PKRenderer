#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Core/YamlSerializers.h"

namespace PK::Core
{
    struct ApplicationConfig : YAML::YamlValueList, public Core::Services::Asset, public Core::Services::IAssetImportSimple
    {
        YAML::BoxedValue<bool> EnableConsole = YAML::BoxedValue<bool>("EnableConsole", true);
        YAML::BoxedValue<bool> EnableVsync = YAML::BoxedValue<bool>("EnableVsync", true);
        YAML::BoxedValue<bool> EnableGizmos = YAML::BoxedValue<bool>("EnableGizmos", false);
        YAML::BoxedValue<bool> EnableLightingDebug = YAML::BoxedValue<bool>("EnableLightingDebug", false);
        YAML::BoxedValue<bool> EnableCursor = YAML::BoxedValue<bool>("EnableCursor", true);
        YAML::BoxedValue<bool> EnableFrameRateLog = YAML::BoxedValue<bool>("EnableFrameRateLog", true);
        YAML::BoxedValue<int> InitialWidth = YAML::BoxedValue<int>("InitialWidth", 1024);
        YAML::BoxedValue<int> InitialHeight = YAML::BoxedValue<int>("InitialHeight", 512);
        YAML::BoxedValue<std::string> FileWindowIcon = YAML::BoxedValue<std::string>("FileWindowIcon", "res/T_AppIcon.bmp");

        YAML::BoxedValue<Math::uint> RandomSeed = YAML::BoxedValue<Math::uint>("RandomSeed", 512);

        YAML::BoxedValue<Math::float3> CameraStartPosition = YAML::BoxedValue<Math::float3>("CameraStartPosition", Math::PK_FLOAT3_ZERO);
        YAML::BoxedValue<Math::float3> CameraStartRotation = YAML::BoxedValue<Math::float3>("CameraStartRotation", Math::PK_FLOAT3_ZERO);
        YAML::BoxedValue<float> CameraSpeed = YAML::BoxedValue<float>("CameraSpeed", 5.0f);
        YAML::BoxedValue<float> CameraLookSensitivity = YAML::BoxedValue<float>("CameraLookSensitivity", 1.0f);
        YAML::BoxedValue<float> CameraMoveSmoothing = YAML::BoxedValue<float>("CameraMoveSmoothing", 0.0f);
        YAML::BoxedValue<float> CameraLookSmoothing = YAML::BoxedValue<float>("CameraLookSmoothing", 0.0f);
        YAML::BoxedValue<float> CameraFov = YAML::BoxedValue<float>("CameraFov", 75.0f);
        YAML::BoxedValue<float> CameraZNear = YAML::BoxedValue<float>("CameraZNear", 0.1f);
        YAML::BoxedValue<float> CameraZFar = YAML::BoxedValue<float>("CameraZFar", 200.0f);
        YAML::BoxedValue<float> CascadeLinearity = YAML::BoxedValue<float>("CascadeLinearity", 0.5f);

        YAML::BoxedValue<float> TimeScale = YAML::BoxedValue<float>("TimeScale", 1.0f);

        YAML::BoxedValue<Math::uint> LightCount = YAML::BoxedValue<Math::uint>("LightCount", 0u);
        YAML::BoxedValue<Math::uint> ShadowmapTileSize = YAML::BoxedValue<Math::uint>("ShadowmapTileSize", 512);

        YAML::BoxedValue<float> CameraFocalLength = YAML::BoxedValue<float>("CameraFocalLength", 0.05f);
        YAML::BoxedValue<float> CameraFNumber = YAML::BoxedValue<float>("CameraFNumber", 1.40f);
        YAML::BoxedValue<float> CameraFilmHeight = YAML::BoxedValue<float>("CameraFilmHeight", 0.024f);
        YAML::BoxedValue<float> CameraFocusSpeed = YAML::BoxedValue<float>("CameraFocusSpeed", 5.0f);
        YAML::BoxedValue<float> AutoExposureLuminanceMin = YAML::BoxedValue<float>("AutoExposureLuminanceMin", 1.0f);
        YAML::BoxedValue<float> AutoExposureLuminanceRange = YAML::BoxedValue<float>("AutoExposureLuminanceRange", 1.0f);
        YAML::BoxedValue<float> AutoExposureSpeed = YAML::BoxedValue<float>("AutoExposureSpeed", 1.0f);
        YAML::BoxedValue<float> TonemapExposure = YAML::BoxedValue<float>("TonemapExposure", 1.0f);
        YAML::BoxedValue<float> VignetteIntensity = YAML::BoxedValue<float>("VignetteIntensity", 15.0f);
        YAML::BoxedValue<float> VignettePower = YAML::BoxedValue<float>("VignettePower", 0.25f);
        YAML::BoxedValue<float> FilmGrainIntensity = YAML::BoxedValue<float>("FilmGrainIntensity", 0.25f);
        YAML::BoxedValue<float> FilmGrainLuminance = YAML::BoxedValue<float>("FilmGrainLuminance", 0.25f);
        YAML::BoxedValue<float> CC_Contribution = YAML::BoxedValue<float>("CC_Contribution", 1.0f);
        YAML::BoxedValue<float> CC_TemperatureShift = YAML::BoxedValue<float>("CC_TemperatureShift", 0.0f);
        YAML::BoxedValue<float> CC_Tint = YAML::BoxedValue<float>("CC_Tint", 0.0f);
        YAML::BoxedValue<float> CC_Hue = YAML::BoxedValue<float>("CC_Hue", 0.0f);
        YAML::BoxedValue<float> CC_Saturation = YAML::BoxedValue<float>("CC_Saturation", 1.0f);
        YAML::BoxedValue<float> CC_Vibrance = YAML::BoxedValue<float>("CC_Vibrance", 0.0f);
        YAML::BoxedValue<float> CC_Value = YAML::BoxedValue<float>("CC_Value", 1.0f);
        YAML::BoxedValue<float> CC_Contrast = YAML::BoxedValue<float>("CC_Contrast", 1.0f);
        YAML::BoxedValue<float> CC_Gain = YAML::BoxedValue<float>("CC_Gain", 1.0f);
        YAML::BoxedValue<float> CC_Gamma = YAML::BoxedValue<float>("CC_Gamma", 1.0f);
        YAML::BoxedValue<Math::uint> CC_Shadows = YAML::BoxedValue<Math::uint>("CC_Shadows", 0x000000FF);
        YAML::BoxedValue<Math::uint> CC_Midtones = YAML::BoxedValue<Math::uint>("CC_Midtones", 0x7F7F7FFF);
        YAML::BoxedValue<Math::uint> CC_Highlights = YAML::BoxedValue<Math::uint>("CC_Highlights", 0xFFFFFFFF);
        YAML::BoxedValue<Math::uint> CC_ChannelMixerRed = YAML::BoxedValue<Math::uint>("CC_ChannelMixerRed", 0xFF0000FF);
        YAML::BoxedValue<Math::uint> CC_ChannelMixerGreen = YAML::BoxedValue<Math::uint>("CC_ChannelMixerGreen", 0x00FF00FF);
        YAML::BoxedValue<Math::uint> CC_ChannelMixerBlue = YAML::BoxedValue<Math::uint>("CC_ChannelMixerBlue", 0x0000FFFF);
        YAML::BoxedValue<float> BloomIntensity = YAML::BoxedValue<float>("BloomIntensity", 0.0f);
        YAML::BoxedValue<float> BloomLensDirtIntensity = YAML::BoxedValue<float>("BloomLensDirtIntensity", 0.0f);
        YAML::BoxedValue<std::string> FileBloomDirt = YAML::BoxedValue<std::string>("FileBloomDirt", "T_Bloom_LensDirt");

        YAML::BoxedValue<bool> GIReSTIR = YAML::BoxedValue<bool>("GIReSTIR", true);
        YAML::BoxedValue<bool> GIApproximateRoughSpecular = YAML::BoxedValue<bool>("GIApproximateRoughSpecular", true);
        YAML::BoxedValue<bool> GIScreenSpacePretrace = YAML::BoxedValue<bool>("GIScreenSpacePretrace", false);
        YAML::BoxedValue<bool> GICheckerboardTrace = YAML::BoxedValue<bool>("GICheckerboardTrace", true);
        YAML::BoxedValue<bool> GISpecularVirtualReproject = YAML::BoxedValue<bool>("GISpecularVirtualReproject", true);

        YAML::BoxedValue<float> TAASharpness = YAML::BoxedValue<float>("TAASharpness", 0.5f);
        YAML::BoxedValue<float> TAABlendingStatic = YAML::BoxedValue<float>("TAABlendingStatic", 0.99f);
        YAML::BoxedValue<float> TAABlendingMotion = YAML::BoxedValue<float>("TAABlendingMotion", 0.85f);
        YAML::BoxedValue<float> TAAMotionAmplification = YAML::BoxedValue<float>("TAAMotionAmplification", 600.0f);

        YAML::BoxedValue<Math::float3> FogAlbedo = YAML::BoxedValue <Math::float3> ("FogAlbedo", Math::PK_FLOAT3_ONE);
        YAML::BoxedValue<Math::float3> FogAbsorption = YAML::BoxedValue <Math::float3> ("FogAbsorption", Math::PK_FLOAT3_ONE);
        YAML::BoxedValue<float> FogAnisotropy = YAML::BoxedValue<float>("FogAnisotropy", 0.0f);
        YAML::BoxedValue<Math::float3> FogWindDirection = YAML::BoxedValue<Math::float3>("FogWindDirection", Math::PK_FLOAT3_FORWARD);
        YAML::BoxedValue<float> FogWindSpeed = YAML::BoxedValue<float>("FogWindSpeed", 0.0f);
        YAML::BoxedValue<float> FogDensityConstant = YAML::BoxedValue<float>("FogDensityConstant", 0.0f);
        YAML::BoxedValue<float> FogDensityHeightExponent = YAML::BoxedValue<float>("FogDensityHeightExponent", 0.0f);
        YAML::BoxedValue<float> FogDensityHeightOffset = YAML::BoxedValue<float>("FogDensityHeightOffset", 0.0f);
        YAML::BoxedValue<float> FogDensityHeightAmount = YAML::BoxedValue<float>("FogDensityHeightAmount", 0.0f);
        YAML::BoxedValue<float> FogDensityNoiseAmount = YAML::BoxedValue<float>("FogDensityNoiseAmount", 0.0f);
        YAML::BoxedValue<float> FogDensityNoiseScale = YAML::BoxedValue<float>("FogDensityNoiseScale", 0.0f);
        YAML::BoxedValue<float> FogDensity = YAML::BoxedValue<float>("FogDensity", 0.0f);

        YAML::BoxedValue<std::string> FileBackgroundTexture = YAML::BoxedValue<std::string>("FileBackgroundTexture", "T_OEM_Mountains");
        YAML::BoxedValue<float> BackgroundExposure = YAML::BoxedValue<float>("BackgroundExposure", 1.0f);

        ApplicationConfig();

        void Import(const char* filepath) final;
    };
}