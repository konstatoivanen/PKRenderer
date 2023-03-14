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
        DECLARE_HASH(_BlurOffset)
        DECLARE_HASH(_Color)
        DECLARE_HASH(pk_Time)
        DECLARE_HASH(pk_SinTime)
        DECLARE_HASH(pk_CosTime)
        DECLARE_HASH(pk_DeltaTime)
        DECLARE_HASH(pk_CursorParams)
        DECLARE_HASH(pk_WorldSpaceCameraPos)
        DECLARE_HASH(pk_ProjectionParams)
        DECLARE_HASH(pk_ExpProjectionParams)
        DECLARE_HASH(pk_ScreenParams)
        DECLARE_HASH(pk_ShadowCascadeZSplits)
        DECLARE_HASH(pk_ProjectionJitter)
        DECLARE_HASH(pk_FrameIndex)
        DECLARE_HASH(pk_MATRIX_M)
        DECLARE_HASH(pk_MATRIX_I_M)
        DECLARE_HASH(pk_MATRIX_V)
        DECLARE_HASH(pk_MATRIX_I_V)
        DECLARE_HASH(pk_MATRIX_P)
        DECLARE_HASH(pk_MATRIX_I_P)
        DECLARE_HASH(pk_MATRIX_VP)
        DECLARE_HASH(pk_MATRIX_I_VP)
        DECLARE_HASH(pk_MATRIX_L_VP)
        DECLARE_HASH(pk_MATRIX_LD_P)
        
        DECLARE_HASH(pk_SceneStructure)

        DECLARE_HASH(pk_SceneOEM_HDR)
        DECLARE_HASH(pk_SceneOEM_Exposure)

        DECLARE_HASH(pk_PerFrameConstants)
        DECLARE_HASH(pk_ModelMatrices)
        DECLARE_HASH(pk_GizmoVertices)
        
        DECLARE_HASH(pk_ScreenDepthCurrent)
        DECLARE_HASH(pk_ScreenDepthPrevious)
        DECLARE_HASH(pk_ScreenNormals)
        DECLARE_HASH(pk_ShadowmapAtlas)
        DECLARE_HASH(pk_LightCookies)
        DECLARE_HASH(pk_Bluenoise256)
        
        DECLARE_HASH(pk_SceneGI_Params)
        DECLARE_HASH(pk_SceneGI_VolumeMaskWrite)
        DECLARE_HASH(pk_SceneGI_VolumeWrite)
        DECLARE_HASH(pk_SceneGI_VolumeRead)
        DECLARE_HASH(pk_ScreenGI_Write)
        DECLARE_HASH(pk_ScreenGI_Read)
        DECLARE_HASH(pk_ScreenGI_Mask)
        DECLARE_HASH(pk_ScreenGI_Hits)
        DECLARE_HASH(pk_ScreenGI_SHY)
        DECLARE_HASH(pk_ScreenGI_CoCg)
        DECLARE_HASH(pk_SceneGI_ST)
        DECLARE_HASH(pk_SceneGI_Swizzle)
        DECLARE_HASH(pk_SceneGI_Checkerboard_Offset)
        DECLARE_HASH(pk_SceneGI_SampleIndex)
        DECLARE_HASH(pk_SceneGI_SampleCount)
        DECLARE_HASH(pk_SceneGI_VoxelSize)
        DECLARE_HASH(pk_SceneGI_ConeAngle)
        DECLARE_HASH(pk_SceneGI_DiffuseGain)
        DECLARE_HASH(pk_SceneGI_SpecularGain)
        DECLARE_HASH(pk_SceneGI_Fade)

        DECLARE_HASH(pk_Lights)
        DECLARE_HASH(pk_LightMatrices)
        DECLARE_HASH(pk_LightDirections)
        DECLARE_HASH(pk_LightCount)
        DECLARE_HASH(pk_GlobalLightsList)
        DECLARE_HASH(pk_LightTiles)
        DECLARE_HASH(pk_GlobalListListIndex)

        DECLARE_HASH(pk_MinLogLuminance)
        DECLARE_HASH(pk_InvLogLuminanceRange)
        DECLARE_HASH(pk_LogLuminanceRange)
        DECLARE_HASH(pk_TargetExposure)
        DECLARE_HASH(pk_AutoExposureSpeed)
        DECLARE_HASH(pk_BloomIntensity)
        DECLARE_HASH(pk_BloomDirtIntensity)
        DECLARE_HASH(pk_BloomTexture)
        DECLARE_HASH(pk_BloomTexture1)

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
        DECLARE_HASH(pk_Foreground)
        DECLARE_HASH(pk_Background)
        DECLARE_HASH(pk_AutoFocusParams)

        DECLARE_HASH(pk_Volume_WindDir)
        DECLARE_HASH(pk_Volume_ConstantFog)
        DECLARE_HASH(pk_Volume_HeightFogExponent)
        DECLARE_HASH(pk_Volume_HeightFogOffset)
        DECLARE_HASH(pk_Volume_HeightFogAmount)
        DECLARE_HASH(pk_Volume_Density)
        DECLARE_HASH(pk_Volume_Intensity)
        DECLARE_HASH(pk_Volume_Anisotropy)
        DECLARE_HASH(pk_Volume_NoiseFogAmount)
        DECLARE_HASH(pk_Volume_NoiseFogScale)
        DECLARE_HASH(pk_Volume_WindSpeed)
        DECLARE_HASH(pk_Volume_ScatterRead)
        DECLARE_HASH(pk_Volume_InjectRead)
        DECLARE_HASH(pk_Volume_Inject)
        DECLARE_HASH(pk_Volume_Scatter)
        DECLARE_HASH(pk_VolumeResources)
        DECLARE_HASH(pk_VolumeMaxDepths)

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