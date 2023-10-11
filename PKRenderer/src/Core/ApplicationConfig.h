#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Core/YamlSerializers.h"

namespace PK::Core
{
    struct ApplicationConfig : YAML::YamlValueList, public Core::Services::Asset, public Core::Services::IAssetImportSimple
    {
        #define DECLARE_VALUE(type, name, defaultValue) YAML::BoxedValue<type> name = YAML::BoxedValue<type>(#name, defaultValue);

        DECLARE_VALUE(bool, EnableConsole, true)
        DECLARE_VALUE(bool, EnableVsync, true)
        DECLARE_VALUE(bool, EnableGizmos, false)
        DECLARE_VALUE(bool, EnableLightingDebug, false)
        DECLARE_VALUE(bool, EnableCursor, true)
        DECLARE_VALUE(bool, EnableFrameRateLog, true)
        DECLARE_VALUE(int, InitialWidth, 1024)
        DECLARE_VALUE(int, InitialHeight, 512)
        DECLARE_VALUE(std::string, FileWindowIcon, "res/T_AppIcon.bmp")

        DECLARE_VALUE(Math::uint, RandomSeed, 512u)

        DECLARE_VALUE(Math::float3, CameraStartPosition, Math::PK_FLOAT3_ZERO)
        DECLARE_VALUE(Math::float3, CameraStartRotation, Math::PK_FLOAT3_ZERO)

        DECLARE_VALUE(float, CameraSpeed, 5.0f)
        DECLARE_VALUE(float, CameraLookSensitivity, 1.0f)
        DECLARE_VALUE(float, CameraMoveSmoothing, 0.0f)
        DECLARE_VALUE(float, CameraLookSmoothing, 0.0f)
        DECLARE_VALUE(float, CameraFov, 75.0f)
        DECLARE_VALUE(float, CameraZNear, 0.1f)
        DECLARE_VALUE(float, CameraZFar, 200.0f)
        DECLARE_VALUE(float, CascadeLinearity, 0.5f)

        DECLARE_VALUE(float, TimeScale, 1.0f)

        DECLARE_VALUE(Math::uint, LightCount, 0u)
        DECLARE_VALUE(Math::uint, ShadowmapTileSize, 512u)

        DECLARE_VALUE(bool, GIReSTIR, true)
        DECLARE_VALUE(bool, GIApproximateRoughSpecular, true)
        DECLARE_VALUE(bool, GIScreenSpacePretrace, false)
        DECLARE_VALUE(bool, GICheckerboardTrace, true)
        DECLARE_VALUE(bool, GISpecularVirtualReproject, true)

        DECLARE_VALUE(float, DoFFocalLength, 0.05f)
        DECLARE_VALUE(float, DoFFNumber, 1.40f)
        DECLARE_VALUE(float, DoFFilmHeight, 0.024f)
        DECLARE_VALUE(float, DoFFocusSpeed, 5.0f)

        DECLARE_VALUE(float, AutoExposureLuminanceMin, 1.0f)
        DECLARE_VALUE(float, AutoExposureLuminanceRange, 1.0f)
        DECLARE_VALUE(float, AutoExposureSpeed, 1.0f)
        DECLARE_VALUE(float, AutoExposureTarget, 1.0f)
        
        DECLARE_VALUE(float, VignetteIntensity, 15.0f)
        DECLARE_VALUE(float, VignettePower, 0.25f)
        
        DECLARE_VALUE(float, FilmGrainIntensity, 0.25f)
        DECLARE_VALUE(float, FilmGrainLuminance, 0.25f)

        DECLARE_VALUE(float, CC_Contribution, 1.0f)
        DECLARE_VALUE(float, CC_TemperatureShift, 0.0f)
        DECLARE_VALUE(float, CC_Tint, 0.0f)
        DECLARE_VALUE(float, CC_Hue, 0.0f)
        DECLARE_VALUE(float, CC_Saturation, 1.0f)
        DECLARE_VALUE(float, CC_Vibrance, 0.0f)
        DECLARE_VALUE(float, CC_Value, 1.0f)
        DECLARE_VALUE(float, CC_Contrast, 1.0f)
        DECLARE_VALUE(float, CC_Gain, 1.0f)
        DECLARE_VALUE(float, CC_Gamma, 1.0f)
        DECLARE_VALUE(Math::uint, CC_Shadows, 0x000000FF)
        DECLARE_VALUE(Math::uint, CC_Midtones, 0x7F7F7FFF)
        DECLARE_VALUE(Math::uint, CC_Highlights, 0xFFFFFFFF)
        DECLARE_VALUE(Math::uint, CC_ChannelMixerRed, 0xFF0000FF)
        DECLARE_VALUE(Math::uint, CC_ChannelMixerGreen, 0x00FF00FF)
        DECLARE_VALUE(Math::uint, CC_ChannelMixerBlue, 0x0000FFFF)

        DECLARE_VALUE(float, BloomIntensity, 0.0f)
        DECLARE_VALUE(float, BloomLensDirtIntensity, 0.0f)
        DECLARE_VALUE(std::string, FileBloomDirt, "T_Bloom_LensDirt")

        DECLARE_VALUE(float, TAASharpness, 0.5f)
        DECLARE_VALUE(float, TAABlendingStatic, 0.99f)
        DECLARE_VALUE(float, TAABlendingMotion, 0.85f)
        DECLARE_VALUE(float, TAAMotionAmplification, 600.0f)

        DECLARE_VALUE(Math::float3, FogAlbedo, Math::PK_FLOAT3_ONE)
        DECLARE_VALUE(Math::float3, FogAbsorption, Math::PK_FLOAT3_ONE)
        DECLARE_VALUE(float, FogPhase0, 0.25f)
        DECLARE_VALUE(float, FogPhase1, 0.95f)
        DECLARE_VALUE(float, FogPhaseW, 0.5f)
        DECLARE_VALUE(Math::float3, FogWindDirection, Math::PK_FLOAT3_FORWARD)
        DECLARE_VALUE(float, FogWindSpeed, 0.0f)
        DECLARE_VALUE(float, FogDensityConstant, 0.0f)
        DECLARE_VALUE(float, FogDensityHeightExponent, 0.0f)
        DECLARE_VALUE(float, FogDensityHeightOffset, 0.0f)
        DECLARE_VALUE(float, FogDensityHeightAmount, 0.0f)
        DECLARE_VALUE(float, FogDensityNoiseAmount, 0.0f)
        DECLARE_VALUE(float, FogDensityNoiseScale, 0.0f)
        DECLARE_VALUE(float, FogDensity, 0.0f)
        DECLARE_VALUE(float, FogDensitySkyConstant, 0.0f)
        DECLARE_VALUE(float, FogDensitySkyHeightExponent, 0.0f)
        DECLARE_VALUE(float, FogDensitySkyHeightOffset, 0.0f)
        DECLARE_VALUE(float, FogDensitySkyHeightAmount, 0.0f)

        DECLARE_VALUE(std::string, FileBackgroundTexture, "T_OEM_Mountains")
        DECLARE_VALUE(float, BackgroundExposure, 1.0f)

        #undef DECLARE_VALUE

        ApplicationConfig();

        void Import(const char* filepath) final;
    };
}