#include "PrecompiledHeader.h"
#include "Core/ApplicationConfig.h"

namespace PK::Core
{
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
			&ZCullLights,
			&LightCount,
			&ShadowmapTileSize,
			&ShadowmapTileCount,
			&CameraFocalLength,
			&CameraFNumber,
			&CameraFilmHeight,
			&CameraFocusSpeed,
			&AutoExposureLuminanceMin,
			&AutoExposureLuminanceRange,
			&AutoExposureSpeed,
			&TonemapExposure,
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
			&BloomIntensity,
			&BloomLensDirtIntensity,
			&FileBloomDirt,
			&AmbientOcclusionIntensity,
			&AmbientOcclusionRadius,
			&AmbientOcclusionDownsample,
			&VolumeConstantFog,
			&VolumeHeightFogExponent,
			&VolumeHeightFogOffset,
			&VolumeHeightFogAmount,
			&VolumeDensity,
			&VolumeIntensity,
			&VolumeAnisotropy,
			&VolumeNoiseFogAmount,
			&VolumeNoiseFogScale,
			&VolumeWindSpeed,
			&VolumeWindDirection,
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
	Ref<ApplicationConfig> AssetImporters::Create() { return CreateRef<ApplicationConfig>(); }
}