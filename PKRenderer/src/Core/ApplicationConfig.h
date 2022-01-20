#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Core/YamlSerializers.h"

namespace PK::Core
{
	using namespace Services;
	using namespace YAML;

	struct ApplicationConfig : YamlValueList, public Asset
	{
		BoxedValue<bool> EnableConsole = BoxedValue<bool>("EnableConsole", true);
		BoxedValue<bool> EnableVsync = BoxedValue<bool>("EnableVsync", true);
		BoxedValue<bool> EnableGizmos = BoxedValue<bool>("EnableGizmos", false);
		BoxedValue<bool> EnableLightingDebug = BoxedValue<bool>("EnableLightingDebug", false);
		BoxedValue<bool> EnableCursor = BoxedValue<bool>("EnableCursor", true);
		BoxedValue<bool> EnableFrameRateLog = BoxedValue<bool>("EnableFrameRateLog", true);
		BoxedValue<int> InitialWidth = BoxedValue<int>("InitialWidth", 1024);
		BoxedValue<int>	InitialHeight = BoxedValue<int>("InitialHeight", 512);
		
		BoxedValue<uint> RandomSeed = BoxedValue<uint>("RandomSeed", 512);

		BoxedValue<float3> CameraStartPosition = BoxedValue<float3>("CameraStartPosition", PK_FLOAT3_ZERO);
		BoxedValue<float3> CameraStartRotation = BoxedValue<float3>("CameraStartRotation", PK_FLOAT3_ZERO);
		BoxedValue<float> CameraSpeed = BoxedValue<float>("CameraSpeed", 5.0f);
		BoxedValue<float> CameraLookSensitivity = BoxedValue<float>("CameraLookSensitivity", 1.0f);
		BoxedValue<float> CameraMoveSmoothing = BoxedValue<float>("CameraMoveSmoothing", 0.0f);
		BoxedValue<float> CameraLookSmoothing = BoxedValue<float>("CameraLookSmoothing", 0.0f);
		BoxedValue<float> CameraFov = BoxedValue<float>("CameraFov", 75.0f);
		BoxedValue<float> CameraZNear = BoxedValue<float>("CameraZNear", 0.1f);
		BoxedValue<float> CameraZFar = BoxedValue<float>("CameraZFar", 200.0f);
		BoxedValue<float> CascadeLinearity = BoxedValue<float>("CascadeLinearity", 0.5f);
		
		BoxedValue<float> TimeScale	= BoxedValue<float>("TimeScale", 1.0f);

		BoxedValue<uint> LightCount = BoxedValue<uint>("LightCount", 0u);
		BoxedValue<uint> ShadowmapTileSize = BoxedValue<uint>("ShadowmapTileSize", 512);
		BoxedValue<uint> ShadowmapTileCount = BoxedValue<uint>("ShadowmapTileCount", 32);
	
		BoxedValue<float> CameraFocalLength	= BoxedValue<float>("CameraFocalLength", 0.05f);
		BoxedValue<float> CameraFNumber	= BoxedValue<float>("CameraFNumber", 1.40f);
		BoxedValue<float> CameraFilmHeight = BoxedValue<float>("CameraFilmHeight", 0.024f);
		BoxedValue<float> CameraFocusSpeed = BoxedValue<float>("CameraFocusSpeed", 5.0f);
		BoxedValue<float> AutoExposureLuminanceMin = BoxedValue<float>("AutoExposureLuminanceMin", 1.0f);
		BoxedValue<float> AutoExposureLuminanceRange = BoxedValue<float>("AutoExposureLuminanceRange", 1.0f);
		BoxedValue<float> AutoExposureSpeed	= BoxedValue<float>("AutoExposureSpeed", 1.0f);
		BoxedValue<float> TonemapExposure = BoxedValue<float>("TonemapExposure", 1.0f);
		BoxedValue<float> VignetteIntensity	= BoxedValue<float>("VignetteIntensity", 15.0f);
		BoxedValue<float> VignettePower	= BoxedValue<float>("VignettePower", 0.25f);
		BoxedValue<float> FilmGrainIntensity = BoxedValue<float>("FilmGrainIntensity", 0.25f);
		BoxedValue<float> FilmGrainLuminance = BoxedValue<float>("FilmGrainLuminance", 0.25f);
		BoxedValue<float> CC_Contribution = BoxedValue<float>("CC_Contribution", 1.0f);
		BoxedValue<float> CC_TemperatureShift = BoxedValue<float>("CC_TemperatureShift", 0.0f);
		BoxedValue<float> CC_Tint = BoxedValue<float>("CC_Tint", 0.0f);
		BoxedValue<float> CC_Hue = BoxedValue<float>("CC_Hue", 0.0f);
		BoxedValue<float> CC_Saturation = BoxedValue<float>("CC_Saturation", 1.0f);
		BoxedValue<float> CC_Vibrance = BoxedValue<float>("CC_Vibrance", 0.0f);
		BoxedValue<float> CC_Value = BoxedValue<float>("CC_Value", 1.0f);
		BoxedValue<float> CC_Contrast = BoxedValue<float>("CC_Contrast", 1.0f);
		BoxedValue<float> CC_Gain = BoxedValue<float>("CC_Gain", 1.0f);
		BoxedValue<float> CC_Gamma = BoxedValue<float>("CC_Gamma", 1.0f);
		BoxedValue<uint> CC_Shadows = BoxedValue<uint>("CC_Shadows", 0x000000FF);
		BoxedValue<uint> CC_Midtones = BoxedValue<uint>("CC_Midtones", 0x7F7F7FFF);
		BoxedValue<uint> CC_Highlights = BoxedValue<uint>("CC_Highlights", 0xFFFFFFFF);
		BoxedValue<uint> CC_ChannelMixerRed = BoxedValue<uint>("CC_ChannelMixerRed", 0xFF0000FF);
		BoxedValue<uint> CC_ChannelMixerGreen = BoxedValue<uint>("CC_ChannelMixerGreen", 0x00FF00FF);
		BoxedValue<uint> CC_ChannelMixerBlue = BoxedValue<uint>("CC_ChannelMixerBlue", 0x0000FFFF);
		BoxedValue<float> BloomIntensity = BoxedValue<float>("BloomIntensity", 0.0f);
		BoxedValue<float> BloomLensDirtIntensity = BoxedValue<float>("BloomLensDirtIntensity", 0.0f);
		BoxedValue<std::string> FileBloomDirt = BoxedValue<std::string>("FileBloomDirt", "T_Bloom_LensDirt");

		BoxedValue<float> AmbientOcclusionIntensity = BoxedValue<float>("AmbientOcclusionIntensity", 1.0f);
		BoxedValue<float> AmbientOcclusionRadius = BoxedValue<float>("AmbientOcclusionRadius", 1.0f);
		BoxedValue<bool> AmbientOcclusionDownsample = BoxedValue<bool>("AmbientOcclusionDownsample", true);

		BoxedValue<float> VolumeConstantFog	= BoxedValue<float>("VolumeConstantFog", 0.0f);
		BoxedValue<float> VolumeHeightFogExponent = BoxedValue<float>("VolumeHeightFogExponent", 0.0f);
		BoxedValue<float> VolumeHeightFogOffset	= BoxedValue<float>("VolumeHeightFogOffset", 0.0f);
		BoxedValue<float> VolumeHeightFogAmount	= BoxedValue<float>("VolumeHeightFogAmount", 0.0f);
		BoxedValue<float> VolumeDensity	= BoxedValue<float>("VolumeDensity", 0.0f);
		BoxedValue<float> VolumeIntensity = BoxedValue<float>("VolumeIntensity", 0.0f);
		BoxedValue<float> VolumeAnisotropy = BoxedValue<float>("VolumeAnisotropy", 0.0f);
		BoxedValue<float> VolumeNoiseFogAmount = BoxedValue<float>("VolumeNoiseFogAmount", 0.0f);
		BoxedValue<float> VolumeNoiseFogScale = BoxedValue<float>("VolumeNoiseFogScale", 0.0f);
		BoxedValue<float> VolumeWindSpeed = BoxedValue<float>("VolumeWindSpeed", 0.0f);
		BoxedValue<float3> VolumeWindDirection = BoxedValue<float3>("VolumeWindDirection", PK_FLOAT3_FORWARD);

		BoxedValue<std::string> FileBackgroundTexture = BoxedValue<std::string>("FileBackgroundTexture", "T_OEM_Mountains");
		BoxedValue<float> BackgroundExposure = BoxedValue<float>("BackgroundExposure", 1.0f);

		ApplicationConfig();

		void Import(const char* filepath, void* pParams) override final;
	};
}