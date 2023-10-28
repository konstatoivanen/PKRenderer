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

        // Generic variable names for generic use cases.
        DECLARE_HASH(pk_Texture)
        DECLARE_HASH(pk_Texture1)
        DECLARE_HASH(pk_Image)
        DECLARE_HASH(pk_Image1)
        DECLARE_HASH(pk_Image2)
        DECLARE_HASH(pk_Image3)
        DECLARE_HASH(pk_Image4)

        DECLARE_HASH(_Offset)
        DECLARE_HASH(_Color)
        DECLARE_HASH(_ColorVoxelize)

        DECLARE_HASH(pk_Time)
        DECLARE_HASH(pk_SinTime)
        DECLARE_HASH(pk_CosTime)
        DECLARE_HASH(pk_DeltaTime)
        DECLARE_HASH(pk_CursorParams)
        DECLARE_HASH(pk_WorldSpaceCameraPos)
        DECLARE_HASH(pk_ViewSpaceCameraDelta)
        DECLARE_HASH(pk_ClipParams)
        DECLARE_HASH(pk_ClipParamsInv)
        DECLARE_HASH(pk_ClipParamsExp)
        DECLARE_HASH(pk_ScreenParams)
        DECLARE_HASH(pk_ShadowCascadeZSplits)
        DECLARE_HASH(pk_ProjectionJitter)
        DECLARE_HASH(pk_FrameRandom)
        DECLARE_HASH(pk_ScreenSize)
        DECLARE_HASH(pk_FrameIndex)

        DECLARE_HASH(pk_ObjectToWorld)
        DECLARE_HASH(pk_WorldToView)
        DECLARE_HASH(pk_ViewToWorld)
        DECLARE_HASH(pk_ViewToWorldPrev)
        DECLARE_HASH(pk_ViewToClip)
        DECLARE_HASH(pk_WorldToClip)
        DECLARE_HASH(pk_WorldToClip_NoJitter)
        DECLARE_HASH(pk_WorldToClipPrev)
        DECLARE_HASH(pk_WorldToClipPrev_NoJitter)
        DECLARE_HASH(pk_ViewToClipDelta)

        DECLARE_HASH(pk_RT_Vertices)
        DECLARE_HASH(pk_RT_Indices)
        
        DECLARE_HASH(pk_Sampler_SurfDefault)
        DECLARE_HASH(pk_Sampler_GBuffer)

        DECLARE_HASH(pk_SceneStructure)

        DECLARE_HASH(pk_SceneEnv)
        DECLARE_HASH(pk_SceneEnv_SH)
        DECLARE_HASH(pk_SceneEnv_Exposure)

        DECLARE_HASH(pk_PerFrameConstants)
        DECLARE_HASH(pk_ModelMatrices)
        DECLARE_HASH(pk_GizmoVertices)
        
        DECLARE_HASH(pk_GB_Current_Normals)
        DECLARE_HASH(pk_GB_Current_Depth)
        DECLARE_HASH(pk_GB_Current_DepthMips)
        DECLARE_HASH(pk_GB_Previous_Color)
        DECLARE_HASH(pk_GB_Previous_Normals)
        DECLARE_HASH(pk_GB_Previous_Depth)

        DECLARE_HASH(pk_ShadowmapAtlas)
        DECLARE_HASH(pk_ShadowmapScreenSpace)
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
        DECLARE_HASH(pk_Reservoirs)

        DECLARE_HASH(pk_Lights)
        DECLARE_HASH(pk_LightMatrices)
        DECLARE_HASH(pk_LightIndex)
        DECLARE_HASH(pk_LightCount)
        DECLARE_HASH(pk_LightLists)
        DECLARE_HASH(pk_LightTiles)

        DECLARE_HASH(pk_PostEffectsParams)
        DECLARE_HASH(pk_PostEffectsFeatureMask)

        DECLARE_HASH(pk_AutoExposure_MinLogLuma)
        DECLARE_HASH(pk_AutoExposure_InvLogLumaRange)
        DECLARE_HASH(pk_AutoExposure_LogLumaRange)
        DECLARE_HASH(pk_AutoExposure_Target)
        DECLARE_HASH(pk_AutoExposure_Speed)
        DECLARE_HASH(pk_AutoExposure_Histogram)

        DECLARE_HASH(pk_Bloom_Intensity)
        DECLARE_HASH(pk_Bloom_DirtIntensity)
        DECLARE_HASH(pk_Bloom_Texture)
        DECLARE_HASH(pk_Bloom_LensDirtTex)

        DECLARE_HASH(pk_Vignette_Intensity)
        DECLARE_HASH(pk_Vignette_Power)

        DECLARE_HASH(pk_FilmGrain_Luminance)
        DECLARE_HASH(pk_FilmGrain_Intensity)
        DECLARE_HASH(pk_FilmGrain_Texture)
        
        DECLARE_HASH(pk_CC_WhiteBalance)
        DECLARE_HASH(pk_CC_Lift)
        DECLARE_HASH(pk_CC_Gamma)
        DECLARE_HASH(pk_CC_Gain)
        DECLARE_HASH(pk_CC_HSV)
        DECLARE_HASH(pk_CC_MixRed)
        DECLARE_HASH(pk_CC_MixGreen)
        DECLARE_HASH(pk_CC_MixBlue)
        DECLARE_HASH(pk_CC_LumaContrast)
        DECLARE_HASH(pk_CC_LumaGain)
        DECLARE_HASH(pk_CC_LumaGamma)
        DECLARE_HASH(pk_CC_Vibrance)
        DECLARE_HASH(pk_CC_Contribution)
        DECLARE_HASH(pk_CC_LutTex)

        DECLARE_HASH(pk_TAA_Sharpness)
        DECLARE_HASH(pk_TAA_BlendingStatic)
        DECLARE_HASH(pk_TAA_BlendingMotion)
        DECLARE_HASH(pk_TAA_MotionAmplification)

        DECLARE_HASH(pk_DoF_FocalLength)
        DECLARE_HASH(pk_DoF_FNumber)
        DECLARE_HASH(pk_DoF_FilmHeight)
        DECLARE_HASH(pk_DoF_FocusSpeed)
        DECLARE_HASH(pk_DoF_MaximumCoC)
        DECLARE_HASH(pk_DoF_Params)
        DECLARE_HASH(pk_DoF_AutoFocusParams)
        DECLARE_HASH(pk_DoF_ColorRead)
        DECLARE_HASH(pk_DoF_ColorWrite)
        DECLARE_HASH(pk_DoF_AlphaRead)
        DECLARE_HASH(pk_DoF_AlphaWrite)

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

        DECLARE_HASH(PK_INSTANCING_ENABLED)
        DECLARE_HASH(PK_META_PASS_GBUFFER)
        DECLARE_HASH(PK_META_PASS_GIVOXELIZE)
        DECLARE_HASH(SHADOW_SOURCE_CUBE)
        DECLARE_HASH(SHADOW_SOURCE_2D)
        DECLARE_HASH(SHADOW_USE_VSM)

        #undef DEFINE_HASH_CACHE

        uint32_t pk_Instancing_Transforms = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TRANSFORMS);
        uint32_t pk_Instancing_Indices = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_INDICES);
        uint32_t pk_Instancing_Properties = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_PROPERTIES);
        uint32_t pk_Instancing_Textures2D = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TEXTURES2D);
        uint32_t pk_Instancing_Textures3D = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TEXTURES3D);
        uint32_t pk_Instancing_TexturesCube = Core::Services::StringHashID::StringToID(PK::Assets::Shader::PK_SHADER_INSTANCING_TEXTURESCUBE);
    };
}