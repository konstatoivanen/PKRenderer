#include "PrecompiledHeader.h"
#include "Core/ApplicationConfig.h"

namespace PK::Core
{
    using namespace PK::Core::Services;

    ApplicationConfig::ApplicationConfig()
    {
        values =
        {
            &EnableConsole,
            &EnableVsync,
            &EnableGizmos,
            &EnableLightingDebug,
            &EnableCursor,
            &EnableFrameRateLog,
            &InitialWidth,
            &InitialHeight,
            &CameraStartPosition,
            &CameraStartRotation,
            &CameraSpeed,
            &CameraLookSensitivity,
            &CameraMoveSmoothing,
            &CameraLookSmoothing,
            &CameraFov,
            &CameraZNear,
            &CameraZFar,
            &CascadeLinearity,
            &TimeScale,
            &RandomSeed,
            &LightCount,
            &ShadowmapTileSize,
            &PostFXApplyVignette,
            &PostFXApplyBloom,
            &PostFXApplyTonemap,
            &PostFXApplyFilmgrain,
            &PostFXApplyColorgrading,
            &PostFXApplyLUTColorGrading,
            &PostFXDebugGIDiff,
            &PostFXDebugGISpec,
            &PostFXDebugGIVX,
            &PostFXDebugNormal,
            &PostFXDebugRoughness,
            &PostFXDebugHalfScreen,
            &PostFXDebugZoom,
            &GIReSTIR,
            &GIApproximateRoughSpecular,
            &GIScreenSpacePretrace,
            &GICheckerboardTrace,
            &GISpecularVirtualReproject,
            &DoFFocalLength,
            &DoFFNumber,
            &DoFFilmHeight,
            &DoFFocusSpeed,
            &AutoExposureLuminanceMin,
            &AutoExposureLuminanceRange,
            &AutoExposureSpeed,
            &AutoExposureTarget,
            &VignetteIntensity,
            &VignettePower,
            &FilmGrainIntensity,
            &FilmGrainLuminance,
            &CC_Contribution,
            &CC_TemperatureShift,
            &CC_Tint,
            &CC_Hue,
            &CC_Saturation,
            &CC_Vibrance,
            &CC_Value,
            &CC_Contrast,
            &CC_Gain,
            &CC_Gamma,
            &CC_Shadows,
            &CC_Midtones,
            &CC_Highlights,
            &CC_ChannelMixerRed,
            &CC_ChannelMixerGreen,
            &CC_ChannelMixerBlue,
            &CC_FileLookupTexture,
            &BloomIntensity,
            &BloomLensDirtIntensity,
            &FileBloomDirt,
            &TAASharpness,
            &TAABlendingStatic,
            &TAABlendingMotion,
            &TAAMotionAmplification,
            &FogAlbedo,
            &FogAbsorption,
            &FogPhase0,
            &FogPhase1,
            &FogPhaseW,
            &FogWindDirection,
            &FogWindSpeed,
            &FogDensityConstant,
            &FogDensityHeightExponent,
            &FogDensityHeightOffset,
            &FogDensityHeightAmount,
            &FogDensityNoiseAmount,
            &FogDensityNoiseScale,
            &FogDensity,
            &FogDensitySkyConstant,
            &FogDensitySkyHeightExponent,
            &FogDensitySkyHeightOffset,
            &FogDensitySkyHeightAmount,
            &FileBackgroundTexture,
            &BackgroundExposure,
        };
    }

    void ApplicationConfig::Import(const char* filepath)
    {
        Load(filepath);
    }

    template<>
    bool AssetImporters::IsValidExtension<ApplicationConfig>(const std::filesystem::path& extension) { return extension.compare(".cfg") == 0; }

    template<>
    Utilities::Ref<ApplicationConfig> AssetImporters::Create() { return Utilities::CreateRef<ApplicationConfig>(); }
}