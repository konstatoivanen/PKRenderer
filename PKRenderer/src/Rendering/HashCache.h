#pragma once
#include "Utilities/ISingleton.h"
#include "Core/Services/IService.h"
#include "Core/Services/StringHashID.h"

namespace PK::Rendering
{
    struct HashCache : public Core::Services::IService, 
                       public Utilities::ISingleton<HashCache>
    {
        #define DECLARE_HASH(name) uint32_t name = Core::Services::StringHashID::StringToID(#name); \

        DECLARE_HASH(_MainTex)
        DECLARE_HASH(_SourceTex)
        DECLARE_HASH(_DestinationTex)
        DECLARE_HASH(_HistoryReadTex)
        DECLARE_HASH(_HistoryWriteTex)
        DECLARE_HASH(_DestinationMip1)
        DECLARE_HASH(_DestinationMip2)
        DECLARE_HASH(_DestinationMip3)
        DECLARE_HASH(_DestinationMip4)
        DECLARE_HASH(_Multiplier)
        DECLARE_HASH(_Color)
        DECLARE_HASH(_ColorVoxelize)
        DECLARE_HASH(pk_Time)
        DECLARE_HASH(pk_SinTime)
        DECLARE_HASH(pk_CosTime)
        DECLARE_HASH(pk_DeltaTime)
        DECLARE_HASH(pk_CursorParams)
        DECLARE_HASH(pk_WorldSpaceCameraPos)
        DECLARE_HASH(pk_ViewSpaceCameraDelta)
        DECLARE_HASH(pk_ProjectionParams)
        DECLARE_HASH(pk_InvProjectionParams)
        DECLARE_HASH(pk_ExpProjectionParams)
        DECLARE_HASH(pk_ScreenParams)
        DECLARE_HASH(pk_ShadowCascadeZSplits)
        DECLARE_HASH(pk_ProjectionJitter)
        DECLARE_HASH(pk_FrameRandom)
        DECLARE_HASH(pk_ScreenSize)
        DECLARE_HASH(pk_FrameIndex)
        DECLARE_HASH(pk_MATRIX_M)
        DECLARE_HASH(pk_MATRIX_I_M)
        DECLARE_HASH(pk_MATRIX_V)
        DECLARE_HASH(pk_MATRIX_I_V)
        DECLARE_HASH(pk_MATRIX_P)
        DECLARE_HASH(pk_MATRIX_VP)
        DECLARE_HASH(pk_MATRIX_VP_N)
        DECLARE_HASH(pk_MATRIX_L_I_V)
        DECLARE_HASH(pk_MATRIX_L_VP)
        DECLARE_HASH(pk_MATRIX_L_VP_N)
        DECLARE_HASH(pk_MATRIX_L_VP_D)
        DECLARE_HASH(pk_RT_Vertices)
        DECLARE_HASH(pk_RT_Indices)
        
        DECLARE_HASH(pk_SceneStructure)

        DECLARE_HASH(pk_SceneEnv)
        DECLARE_HASH(pk_SceneEnv_SH)
        DECLARE_HASH(pk_SceneEnv_Exposure)

        DECLARE_HASH(pk_PerFrameConstants)
        DECLARE_HASH(pk_ModelMatrices)
        DECLARE_HASH(pk_GizmoVertices)
        
        DECLARE_HASH(pk_ScreenDepthCurrent)
        DECLARE_HASH(pk_ScreenDepthHierachical)
        DECLARE_HASH(pk_ScreenDepthPrevious)
        DECLARE_HASH(pk_ScreenNormalsCurrent)
        DECLARE_HASH(pk_ScreenNormalsPrevious)
        DECLARE_HASH(pk_ScreenColorPrevious)
        DECLARE_HASH(pk_ShadowmapAtlas)
        DECLARE_HASH(pk_LightCookies)
        DECLARE_HASH(pk_Bluenoise256)
        DECLARE_HASH(pk_Bluenoise128x64)
        
        DECLARE_HASH(pk_GI_Parameters)
        DECLARE_HASH(pk_GI_VolumeMaskWrite)
        DECLARE_HASH(pk_GI_VolumeWrite)
        DECLARE_HASH(pk_GI_VolumeRead)
        DECLARE_HASH(pk_GI_PackedDiff)
        DECLARE_HASH(pk_GI_PackedSpec)
        DECLARE_HASH(pk_GI_ResolvedWrite)
        DECLARE_HASH(pk_GI_ResolvedRead)
        DECLARE_HASH(pk_GI_RayHits)
        DECLARE_HASH(pk_GI_VolumeST)
        DECLARE_HASH(pk_GI_VolumeSwizzle)
        DECLARE_HASH(pk_GI_RayDither)
        DECLARE_HASH(pk_GI_VoxelSize)
        DECLARE_HASH(pk_GI_ChromaBias)
        DECLARE_HASH(pk_Reservoirs)

        DECLARE_HASH(pk_Lights)
        DECLARE_HASH(pk_LightMatrices)
        DECLARE_HASH(pk_LightCount)
        DECLARE_HASH(pk_GlobalLightsList)
        DECLARE_HASH(pk_LightTiles)

        DECLARE_HASH(pk_MinLogLuminance)
        DECLARE_HASH(pk_InvLogLuminanceRange)
        DECLARE_HASH(pk_LogLuminanceRange)
        DECLARE_HASH(pk_TargetExposure)
        DECLARE_HASH(pk_AutoExposureSpeed)
        DECLARE_HASH(pk_BloomIntensity)
        DECLARE_HASH(pk_BloomDirtIntensity)
        DECLARE_HASH(pk_BloomTexture)

        DECLARE_HASH(pk_Vibrance)
        DECLARE_HASH(pk_VignetteGrain)
        DECLARE_HASH(pk_WhiteBalance)
        DECLARE_HASH(pk_Lift)
        DECLARE_HASH(pk_Gamma)
        DECLARE_HASH(pk_Gain)
        DECLARE_HASH(pk_ContrastGainGammaContribution)
        DECLARE_HASH(pk_HSV)
        DECLARE_HASH(pk_ChannelMixerRed)
        DECLARE_HASH(pk_ChannelMixerGreen)
        DECLARE_HASH(pk_ChannelMixerBlue)
        DECLARE_HASH(pk_FilmGrainTex)
        DECLARE_HASH(pk_BloomLensDirtTex)
        DECLARE_HASH(pk_PostEffectsParams)
        DECLARE_HASH(pk_Histogram)

        DECLARE_HASH(pk_TAA_Sharpness)
        DECLARE_HASH(pk_TAA_BlendingStatic)
        DECLARE_HASH(pk_TAA_BlendingMotion)
        DECLARE_HASH(pk_TAA_MotionAmplification)

        DECLARE_HASH(pk_FocalLength)
        DECLARE_HASH(pk_FNumber)
        DECLARE_HASH(pk_FilmHeight)
        DECLARE_HASH(pk_FocusSpeed)
        DECLARE_HASH(pk_MaximumCoC)
        DECLARE_HASH(pk_DofParams)
        DECLARE_HASH(pk_AutoFocusParams)
        DECLARE_HASH(pk_DoFColorRead)
        DECLARE_HASH(pk_DoFColorWrite)
        DECLARE_HASH(pk_DoFAlphaRead)
        DECLARE_HASH(pk_DoFAlphaWrite)


        DECLARE_HASH(pk_Fog_Albedo)
        DECLARE_HASH(pk_Fog_Absorption)
        DECLARE_HASH(pk_Fog_WindDirSpeed)
        DECLARE_HASH(pk_Fog_Phase0)
        DECLARE_HASH(pk_Fog_Phase1)
        DECLARE_HASH(pk_Fog_PhaseW)
        DECLARE_HASH(pk_Fog_Density_Constant)
        DECLARE_HASH(pk_Fog_Density_HeightExponent)
        DECLARE_HASH(pk_Fog_Density_HeightOffset)
        DECLARE_HASH(pk_Fog_Density_HeightAmount)
        DECLARE_HASH(pk_Fog_Density_NoiseAmount)
        DECLARE_HASH(pk_Fog_Density_NoiseScale)
        DECLARE_HASH(pk_Fog_Density_Amount)
        DECLARE_HASH(pk_Fog_Density_Sky_Constant)
        DECLARE_HASH(pk_Fog_Density_Sky_HeightExponent)
        DECLARE_HASH(pk_Fog_Density_Sky_HeightOffset)
        DECLARE_HASH(pk_Fog_Density_Sky_HeightAmount)
        DECLARE_HASH(pk_Fog_InjectRead)
        DECLARE_HASH(pk_Fog_DensityRead)
        DECLARE_HASH(pk_Fog_ScatterRead)
        DECLARE_HASH(pk_Fog_TransmittanceRead)
        DECLARE_HASH(pk_Fog_Inject)
        DECLARE_HASH(pk_Fog_Density)
        DECLARE_HASH(pk_Fog_Scatter)
        DECLARE_HASH(pk_Fog_Transmittance)
        DECLARE_HASH(pk_Fog_Parameters)

        DECLARE_HASH(pk_ShadowmapSource)
        DECLARE_HASH(pk_ShadowmapData)

        DECLARE_HASH(PK_INSTANCING_ENABLED)
        DECLARE_HASH(PK_META_PASS_GBUFFER)
        DECLARE_HASH(PK_META_PASS_GIVOXELIZE)
        DECLARE_HASH(SHADOW_SOURCE_CUBE)
        DECLARE_HASH(SHADOW_SOURCE_2D)
        DECLARE_HASH(SHADOW_BLUR_PASS0)
        DECLARE_HASH(SHADOW_BLUR_PASS1)

        #undef DEFINE_HASH_CACHE

        uint32_t pk_Instancing_Transforms = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TRANSFORMS);
        uint32_t pk_Instancing_Indices = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_INDICES);
        uint32_t pk_Instancing_Properties = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_PROPERTIES);
        uint32_t pk_Instancing_Textures2D = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TEXTURES2D);
        uint32_t pk_Instancing_Textures3D = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TEXTURES3D);
        uint32_t pk_Instancing_TexturesCube = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TEXTURESCUBE);
    };
}