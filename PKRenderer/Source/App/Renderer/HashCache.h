#pragma once
#include <PKAssets/PKAsset.h>
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/NameID.h"

namespace PK::App
{
    struct HashCache : public ISingleton<HashCache>
    {
#define DECLARE_HASH(name) NameID name = #name; \

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
        DECLARE_HASH(pk_ViewWorldOrigin)
        DECLARE_HASH(pk_ViewWorldOriginPrev)
        DECLARE_HASH(pk_ViewSpaceCameraDelta)
        DECLARE_HASH(pk_ClipParams)
        DECLARE_HASH(pk_ClipParamsInv)
        DECLARE_HASH(pk_ScreenParams)
        DECLARE_HASH(pk_ProjectionJitter)
        DECLARE_HASH(pk_FrameRandom)
        DECLARE_HASH(pk_ScreenSize)
        DECLARE_HASH(pk_FrameIndex)
        
        DECLARE_HASH(pk_MeshletCullParams)
        DECLARE_HASH(pk_ShadowCascadeZSplits)
        DECLARE_HASH(pk_LightTileZParams)

        DECLARE_HASH(pk_ObjectToWorld)
        DECLARE_HASH(pk_WorldToView)
        DECLARE_HASH(pk_ViewToWorld)
        DECLARE_HASH(pk_ViewToWorldPrev)
        DECLARE_HASH(pk_ViewToClip)
        DECLARE_HASH(pk_WorldToClip)
        DECLARE_HASH(pk_WorldToClip_NoJitter)
        DECLARE_HASH(pk_WorldToClipPrev)
        DECLARE_HASH(pk_WorldToClipPrev_NoJitter)
        DECLARE_HASH(pk_ViewToPrevClip)
        DECLARE_HASH(pk_ClipToPrevClip_NoJitter)

        DECLARE_HASH(pk_Sampler_SurfDefault)
        DECLARE_HASH(pk_Sampler_GBuffer)
        DECLARE_HASH(pk_Sampler_GUI)

        DECLARE_HASH(pk_SceneStructure)

        DECLARE_HASH(pk_PreIntegratedDFG)
        DECLARE_HASH(pk_SceneEnv)
        DECLARE_HASH(pk_SceneEnv_ISL)
        DECLARE_HASH(pk_SceneEnv_Origin)
        DECLARE_HASH(pk_SceneEnv_SH)
        DECLARE_HASH(pk_SceneEnv_Exposure)

        DECLARE_HASH(pk_PerFrameConstants)
        DECLARE_HASH(pk_ModelMatrices)
        DECLARE_HASH(pk_GizmoVertices)

        DECLARE_HASH(pk_GB_Current_Normals)
        DECLARE_HASH(pk_GB_Current_Depth)
        DECLARE_HASH(pk_GB_Current_DepthBiased)
        DECLARE_HASH(pk_GB_Current_DepthMips)
        DECLARE_HASH(pk_GB_Previous_Color)
        DECLARE_HASH(pk_GB_Previous_Normals)
        DECLARE_HASH(pk_GB_Previous_Depth)
        DECLARE_HASH(pk_GB_Previous_DepthBiased)

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
        DECLARE_HASH(pk_GI_VoxelStepSize)
        DECLARE_HASH(pk_GI_VoxelLevelScale)
        DECLARE_HASH(pk_Reservoirs0)
        DECLARE_HASH(pk_Reservoirs1)

        DECLARE_HASH(pk_Lights)
        DECLARE_HASH(pk_LightMatrices)
        DECLARE_HASH(pk_LightCount)
        DECLARE_HASH(pk_LightLists)
        DECLARE_HASH(pk_LightTiles)
        DECLARE_HASH(pk_BendShadowDispatchData)

        DECLARE_HASH(pk_PostEffectsFeatureMask)

        DECLARE_HASH(pk_AutoExposure_LogLumaRange)
        DECLARE_HASH(pk_AutoExposure_Min)
        DECLARE_HASH(pk_AutoExposure_Max)
        DECLARE_HASH(pk_AutoExposure_Target)
        DECLARE_HASH(pk_AutoExposure_Speed)
        DECLARE_HASH(pk_AutoExposure_Histogram)

        DECLARE_HASH(pk_Bloom_Diffusion)
        DECLARE_HASH(pk_Bloom_Intensity)
        DECLARE_HASH(pk_Bloom_DirtIntensity)
        DECLARE_HASH(pk_Bloom_UpsampleLayerCount)
        DECLARE_HASH(pk_Bloom_Texture)
        DECLARE_HASH(pk_Bloom_LensDirtTex)

        DECLARE_HASH(pk_Vignette_Intensity)
        DECLARE_HASH(pk_Vignette_Power)

        DECLARE_HASH(pk_FilmGrain_Luminance)
        DECLARE_HASH(pk_FilmGrain_Intensity)
        DECLARE_HASH(pk_FilmGrain_ExposureSensitivity)
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
        DECLARE_HASH(pk_Tonemap_LutTex)

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
        DECLARE_HASH(pk_DoF_AutoFocusState)
        DECLARE_HASH(pk_DoF_ColorRead)
        DECLARE_HASH(pk_DoF_ColorWrite)
        DECLARE_HASH(pk_DoF_AlphaRead)
        DECLARE_HASH(pk_DoF_AlphaWrite)

        DECLARE_HASH(pk_Fog_ZParams)
        DECLARE_HASH(pk_Fog_FadeParams)
        DECLARE_HASH(pk_Fog_Albedo)
        DECLARE_HASH(pk_Fog_Absorption)
        DECLARE_HASH(pk_Fog_WindDirSpeed)
        DECLARE_HASH(pk_Fog_Phase0)
        DECLARE_HASH(pk_Fog_Phase1)
        DECLARE_HASH(pk_Fog_PhaseW)
        DECLARE_HASH(pk_Fog_Density_NoiseAmount)
        DECLARE_HASH(pk_Fog_Density_NoiseScale)
        DECLARE_HASH(pk_Fog_Density_Amount)
        DECLARE_HASH(pk_Fog_Density_ExpParams0)
        DECLARE_HASH(pk_Fog_Density_ExpParams1)
        DECLARE_HASH(pk_Fog_InjectRead)
        DECLARE_HASH(pk_Fog_DensityRead)
        DECLARE_HASH(pk_Fog_ScatterRead)
        DECLARE_HASH(pk_Fog_Inject)
        DECLARE_HASH(pk_Fog_Density)
        DECLARE_HASH(pk_Fog_Scatter)

        DECLARE_HASH(pk_Meshlet_Tasklets)
        DECLARE_HASH(pk_Meshlet_DispatchOffset)
        DECLARE_HASH(pk_Meshlet_Submeshes)
        DECLARE_HASH(pk_Meshlets)
        DECLARE_HASH(pk_Meshlet_Vertices)
        DECLARE_HASH(pk_Meshlet_Indices)

        DECLARE_HASH(pk_Gizmos_IndirectVertices)
        DECLARE_HASH(pk_Gizmos_IndirectArguments)
        DECLARE_HASH(pk_GUI_Vertices)
        DECLARE_HASH(pk_GUI_Textures)

        DECLARE_HASH(PK_LIGHT_PASS_DIRECTIONAL)
        DECLARE_HASH(PK_LIGHT_PASS_SPOT)
        DECLARE_HASH(PK_LIGHT_PASS_POINT)

        DECLARE_HASH(PK_INSTANCING_ENABLED)
        DECLARE_HASH(PK_META_PASS_GBUFFER)
        DECLARE_HASH(PK_META_PASS_GIVOXELIZE)

#undef DEFINE_HASH_CACHE

        NameID pk_Instancing_Transforms = NameID(PKAssets::PK_SHADER_INSTANCING_TRANSFORMS);
        NameID pk_Instancing_Indices = NameID(PKAssets::PK_SHADER_INSTANCING_INDICES);
        NameID pk_Instancing_Properties = NameID(PKAssets::PK_SHADER_INSTANCING_PROPERTIES);
        NameID pk_Instancing_Textures2D = NameID(PKAssets::PK_SHADER_INSTANCING_TEXTURES2D);
        NameID pk_Instancing_Textures3D = NameID(PKAssets::PK_SHADER_INSTANCING_TEXTURES3D);
        NameID pk_Instancing_TexturesCube = NameID(PKAssets::PK_SHADER_INSTANCING_TEXTURESCUBE);
    };
}