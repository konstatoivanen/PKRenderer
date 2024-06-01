#pragma once
#include "Core/Math/Math.h"
#include "Core/Yaml/ConvertMathTypes.h"
#include "Core/Yaml/ConfigMacros.h"

namespace PK::App
{
    PK_YAML_ASSET_BEGIN(RendererConfig, ".cfg")
        PK_YAML_MEMBER(bool, EnableConsole, true)
        PK_YAML_MEMBER(bool, EnableLogRHI, false)
        PK_YAML_MEMBER(bool, EnableLogVerbose, true)
        PK_YAML_MEMBER(bool, EnableLogInfo, true)
        PK_YAML_MEMBER(bool, EnableLogWarning, true)
        PK_YAML_MEMBER(bool, EnableLogError, true)
        PK_YAML_MEMBER(bool, EnableVsync, true)
        PK_YAML_MEMBER(bool, EnableGizmosCPU, false)
        PK_YAML_MEMBER(bool, EnableGizmosGPU, false)
        PK_YAML_MEMBER(bool, EnableLightingDebug, false)
        PK_YAML_MEMBER(bool, EnableCursor, true)
        PK_YAML_MEMBER(bool, EnableFrameRateLog, true)
        PK_YAML_MEMBER(int, InitialWidth, 1024)
        PK_YAML_MEMBER(int, InitialHeight, 512)
        PK_YAML_MEMBER(std::string, FileWindowIcon, "Content/T_AppIcon.bmp")

        PK_YAML_MEMBER(uint, RandomSeed, 512u)

        PK_YAML_MEMBER(float3, CameraStartPosition, PK_FLOAT3_ZERO)
        PK_YAML_MEMBER(float3, CameraStartRotation, PK_FLOAT3_ZERO)

        PK_YAML_MEMBER(float, CameraSpeed, 5.0f)
        PK_YAML_MEMBER(float, CameraLookSensitivity, 1.0f)
        PK_YAML_MEMBER(float, CameraMoveSmoothing, 0.0f)
        PK_YAML_MEMBER(float, CameraLookSmoothing, 0.0f)
        PK_YAML_MEMBER(float, CameraFov, 75.0f)
        PK_YAML_MEMBER(float, CameraZNear, 0.1f)
        PK_YAML_MEMBER(float, CameraZFar, 200.0f)
        PK_YAML_MEMBER(float, CascadeLinearity, 0.5f)

        PK_YAML_MEMBER(float, TimeScale, 1.0f)

        PK_YAML_MEMBER(uint, LightCount, 0u)
        PK_YAML_MEMBER(uint, ShadowmapTileSize, 512u)

        PK_YAML_MEMBER(bool, PostFXApplyVignette, true)
        PK_YAML_MEMBER(bool, PostFXApplyBloom, true)
        PK_YAML_MEMBER(bool, PostFXApplyTonemap, true)
        PK_YAML_MEMBER(bool, PostFXApplyFilmgrain, true)
        PK_YAML_MEMBER(bool, PostFXApplyColorgrading, false)
        PK_YAML_MEMBER(bool, PostFXApplyLUTColorGrading, true)

        PK_YAML_MEMBER(bool, PostFXDebugGIDiff, false)
        PK_YAML_MEMBER(bool, PostFXDebugGISpec, false)
        PK_YAML_MEMBER(bool, PostFXDebugGIVX, false)
        PK_YAML_MEMBER(bool, PostFXDebugNormal, false)
        PK_YAML_MEMBER(bool, PostFXDebugRoughness, false)
        PK_YAML_MEMBER(bool, PostFXDebugHalfScreen, false)
        PK_YAML_MEMBER(bool, PostFXDebugZoom, false)

        PK_YAML_MEMBER(bool, GIReSTIR, true)
        PK_YAML_MEMBER(bool, GIApproximateRoughSpecular, true)
        PK_YAML_MEMBER(bool, GIScreenSpacePretrace, false)
        PK_YAML_MEMBER(bool, GICheckerboardTrace, true)
        PK_YAML_MEMBER(bool, GISpecularVirtualReproject, true)

        PK_YAML_MEMBER(float, DoFFocalLength, 0.05f)
        PK_YAML_MEMBER(float, DoFFNumber, 1.40f)
        PK_YAML_MEMBER(float, DoFFilmHeight, 0.024f)
        PK_YAML_MEMBER(float, DoFFocusSpeed, 5.0f)

        PK_YAML_MEMBER(float, AutoExposureLuminanceMin, 1.0f)
        PK_YAML_MEMBER(float, AutoExposureLuminanceRange, 1.0f)
        PK_YAML_MEMBER(float, AutoExposureSpeed, 1.0f)
        PK_YAML_MEMBER(float, AutoExposureTarget, 1.0f)

        PK_YAML_MEMBER(float, VignetteIntensity, 15.0f)
        PK_YAML_MEMBER(float, VignettePower, 0.25f)

        PK_YAML_MEMBER(float, FilmGrainIntensity, 0.25f)
        PK_YAML_MEMBER(float, FilmGrainLuminance, 0.25f)

        PK_YAML_MEMBER(float, CC_Contribution, 1.0f)
        PK_YAML_MEMBER(float, CC_TemperatureShift, 0.0f)
        PK_YAML_MEMBER(float, CC_Tint, 0.0f)
        PK_YAML_MEMBER(float, CC_Hue, 0.0f)
        PK_YAML_MEMBER(float, CC_Saturation, 1.0f)
        PK_YAML_MEMBER(float, CC_Vibrance, 0.0f)
        PK_YAML_MEMBER(float, CC_Value, 1.0f)
        PK_YAML_MEMBER(float, CC_Contrast, 1.0f)
        PK_YAML_MEMBER(float, CC_Gain, 1.0f)
        PK_YAML_MEMBER(float, CC_Gamma, 1.0f)
        PK_YAML_MEMBER(uint, CC_Shadows, 0x000000FF)
        PK_YAML_MEMBER(uint, CC_Midtones, 0x7F7F7FFF)
        PK_YAML_MEMBER(uint, CC_Highlights, 0xFFFFFFFF)
        PK_YAML_MEMBER(uint, CC_ChannelMixerRed, 0xFF0000FF)
        PK_YAML_MEMBER(uint, CC_ChannelMixerGreen, 0x00FF00FF)
        PK_YAML_MEMBER(uint, CC_ChannelMixerBlue, 0x0000FFFF)
        PK_YAML_MEMBER(std::string, CC_FileLookupTexture, "T_CC_LUT32")

        PK_YAML_MEMBER(float, BloomIntensity, 0.0f)
        PK_YAML_MEMBER(float, BloomLensDirtIntensity, 0.0f)
        PK_YAML_MEMBER(std::string, FileBloomDirt, "T_Bloom_LensDirt")

        PK_YAML_MEMBER(float, TAASharpness, 0.5f)
        PK_YAML_MEMBER(float, TAABlendingStatic, 0.99f)
        PK_YAML_MEMBER(float, TAABlendingMotion, 0.85f)
        PK_YAML_MEMBER(float, TAAMotionAmplification, 600.0f)

        PK_YAML_MEMBER(float3, FogAlbedo, PK_FLOAT3_ONE)
        PK_YAML_MEMBER(float3, FogAbsorption, PK_FLOAT3_ONE)
        PK_YAML_MEMBER(float, FogPhase0, 0.25f)
        PK_YAML_MEMBER(float, FogPhase1, 0.95f)
        PK_YAML_MEMBER(float, FogPhaseW, 0.5f)
        PK_YAML_MEMBER(float3, FogWindDirection, PK_FLOAT3_FORWARD)
        PK_YAML_MEMBER(float, FogWindSpeed, 0.0f)
        PK_YAML_MEMBER(float, FogDensityConstant, 0.0f)
        PK_YAML_MEMBER(float, FogDensityHeightExponent, 0.0f)
        PK_YAML_MEMBER(float, FogDensityHeightOffset, 0.0f)
        PK_YAML_MEMBER(float, FogDensityHeightAmount, 0.0f)
        PK_YAML_MEMBER(float, FogDensityNoiseAmount, 0.0f)
        PK_YAML_MEMBER(float, FogDensityNoiseScale, 0.0f)
        PK_YAML_MEMBER(float, FogDensity, 0.0f)
        PK_YAML_MEMBER(float, FogDensitySkyConstant, 0.0f)
        PK_YAML_MEMBER(float, FogDensitySkyHeightExponent, 0.0f)
        PK_YAML_MEMBER(float, FogDensitySkyHeightOffset, 0.0f)
        PK_YAML_MEMBER(float, FogDensitySkyHeightAmount, 0.0f)

        PK_YAML_MEMBER(std::string, FileBackgroundTexture, "T_OEM_Mountains")
        PK_YAML_MEMBER(float, BackgroundExposure, 1.0f)
        PK_YAML_ASSET_END()
}

PK_YAML_ASSET_ASSETDATABSE_INTERFACE(PK::App::RendererConfig)
