#pragma once
#include <PKAssets/PKAsset.h>
#include "Core/Base/Types/Singleton.h"
#include "Core/Base/Types/NameID.h"

namespace PK::App
{
    struct HashCache : public Singleton<HashCache>
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
        DECLARE_HASH(pk_Image5)
        DECLARE_HASH(pk_Image6)
        DECLARE_HASH(pk_ImageArray)

        DECLARE_HASH(pk_HZB_WorkgroupCounter)
        DECLARE_HASH(pk_HZB_Parameters)

        DECLARE_HASH(_Offset)
        DECLARE_HASH(_Color)
        DECLARE_HASH(_ColorVoxelize)

        // PKSceneView Parameters
        DECLARE_HASH(WorldToView)
        DECLARE_HASH(ViewToWorld)
        DECLARE_HASH(ViewToWorldPrev)
        DECLARE_HASH(ViewToClip)
        DECLARE_HASH(WorldToClip)
        DECLARE_HASH(WorldToClip_NoJitter)
        DECLARE_HASH(WorldToClipPrev)
        DECLARE_HASH(WorldToClipPrev_NoJitter)
        DECLARE_HASH(ViewToPrevClip)
        DECLARE_HASH(ClipToPrevClip_NoJitter)
        DECLARE_HASH(Time)
        DECLARE_HASH(SinTime)
        DECLARE_HASH(CosTime)
        DECLARE_HASH(DeltaTime)
        DECLARE_HASH(CursorParams)
        DECLARE_HASH(ViewWorldOrigin)
        DECLARE_HASH(ViewWorldOriginPrev)
        DECLARE_HASH(ViewSpaceCameraDelta)
        DECLARE_HASH(ClipParams)
        DECLARE_HASH(ClipParamsInv)
        DECLARE_HASH(ScreenParams)
        DECLARE_HASH(ProjectionJitter)
        DECLARE_HASH(FrameRandom)
        DECLARE_HASH(ScreenSize)
        DECLARE_HASH(FrameIndex)
        DECLARE_HASH(MeshletCullParams)
        DECLARE_HASH(ShadowCascadeZSplits)
        DECLARE_HASH(LightTileZParams)
        DECLARE_HASH(ScreenLevels)
        DECLARE_HASH(GI_VX_TexelSize)
        DECLARE_HASH(GI_VX_StepSize)
        DECLARE_HASH(GI_VX_Swizzle)
        DECLARE_HASH(GI_VX_LevelScale)
        DECLARE_HASH(GI_VX_ST)
        DECLARE_HASH(GI_RayDither)
        DECLARE_HASH(Fog_Density_NoiseAmount)
        DECLARE_HASH(Fog_Density_NoiseScale)
        DECLARE_HASH(Fog_Density_Amount)
        DECLARE_HASH(Fog_Phase0)
        DECLARE_HASH(Fog_Phase1)
        DECLARE_HASH(Fog_PhaseW)
        DECLARE_HASH(Fog_Albedo)
        DECLARE_HASH(Fog_ZParams)
        DECLARE_HASH(Fog_FadeParams)
        DECLARE_HASH(Fog_Absorption)
        DECLARE_HASH(Fog_WindDirSpeed)
        DECLARE_HASH(Fog_Density_ExpParams0)
        DECLARE_HASH(Fog_Density_ExpParams1)
        DECLARE_HASH(Panini_Projection_Parameters)
        DECLARE_HASH(CC_WhiteBalance)
        DECLARE_HASH(CC_Lift)
        DECLARE_HASH(CC_Gamma)
        DECLARE_HASH(CC_Gain)
        DECLARE_HASH(CC_HSV)
        DECLARE_HASH(CC_MixRed)
        DECLARE_HASH(CC_MixGreen)
        DECLARE_HASH(CC_MixBlue)
        DECLARE_HASH(CC_LumaContrast)
        DECLARE_HASH(CC_LumaGain)
        DECLARE_HASH(CC_LumaGamma)
        DECLARE_HASH(CC_Vibrance)
        DECLARE_HASH(CC_Contribution)
        DECLARE_HASH(Vignette_Intensity)
        DECLARE_HASH(Vignette_Power)
        DECLARE_HASH(FilmGrain_Luminance)
        DECLARE_HASH(FilmGrain_Intensity)
        DECLARE_HASH(FilmGrain_ExposureSensitivity)
        DECLARE_HASH(AutoExposure_LogLumaRange)
        DECLARE_HASH(AutoExposure_Target)
        DECLARE_HASH(AutoExposure_Min)
        DECLARE_HASH(AutoExposure_Max)
        DECLARE_HASH(AutoExposure_Speed)
        DECLARE_HASH(Bloom_Diffusion)
        DECLARE_HASH(Bloom_Intensity)
        DECLARE_HASH(Bloom_DirtIntensity)
        DECLARE_HASH(Chromatic_Aberration_Amount)
        DECLARE_HASH(Chromatic_Aberration_Power)
        DECLARE_HASH(TAA_Sharpness)
        DECLARE_HASH(TAA_BlendingStatic)
        DECLARE_HASH(TAA_BlendingMotion)
        DECLARE_HASH(TAA_MotionAmplification)
        DECLARE_HASH(PostEffectsFeatureMask)
        /*
        DECLARE_HASH(SamplerBilinearRepeat)
        DECLARE_HASH(SamplerBilinearMirror)
        DECLARE_HASH(SamplerBilinearClamped)
        DECLARE_HASH(SamplerTrilinearBorder)
        DECLARE_HASH(SamplerTrilinearClamped)
        DECLARE_HASH(SamplerTrilinearRepeatAniso)
        DECLARE_HASH(SamplerPointClamped)
        */
        DECLARE_HASH(pk_View)

        DECLARE_HASH(pk_SamplerBilinearRepeat)
        DECLARE_HASH(pk_SamplerBilinearMirror)
        DECLARE_HASH(pk_SamplerBilinearClamped)
        DECLARE_HASH(pk_SamplerTrilinearBorder)
        DECLARE_HASH(pk_SamplerTrilinearClamped)
        DECLARE_HASH(pk_SamplerTrilinearRepeatAniso)
        DECLARE_HASH(pk_SamplerPointClamped)


        DECLARE_HASH(pk_SceneStructure)
        DECLARE_HASH(pk_ObjectToWorld)

        DECLARE_HASH(pk_PreIntegratedDFG)
        DECLARE_HASH(pk_SceneEnv)
        DECLARE_HASH(pk_SceneEnv_ISL)
        DECLARE_HASH(pk_SceneEnv_Origin)
        DECLARE_HASH(pk_SceneEnv_SH)
        DECLARE_HASH(pk_SceneEnv_Exposure)

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
        DECLARE_HASH(pk_IESProfiles)
        DECLARE_HASH(pk_Bluenoise256)
        DECLARE_HASH(pk_Bluenoise128x64)

        DECLARE_HASH(pk_GI_Parameters)
        DECLARE_HASH(pk_GI_PackedDiff)
        DECLARE_HASH(pk_GI_PackedSpec)
        DECLARE_HASH(pk_GI_ResolvedWrite)
        DECLARE_HASH(pk_GI_ResolvedRead)
        DECLARE_HASH(pk_GI_RayHits)
        DECLARE_HASH(pk_Reservoirs0)
        DECLARE_HASH(pk_Reservoirs1)
        DECLARE_HASH(pk_GI_VX_Mask)
        DECLARE_HASH(pk_GI_VX_RadianceWrite)
        DECLARE_HASH(pk_GI_VX_RadianceRead)

        DECLARE_HASH(pk_Lights)
        DECLARE_HASH(pk_LightCounter)
        DECLARE_HASH(pk_LightMatrices)
        DECLARE_HASH(pk_LastLightIndex)
        DECLARE_HASH(pk_LightLists)
        DECLARE_HASH(pk_LightTiles)
        DECLARE_HASH(pk_LightCoordinate)
        DECLARE_HASH(pk_WaveOffset)

        DECLARE_HASH(pk_AutoExposure_Histogram)

        DECLARE_HASH(pk_Bloom_UpsampleWeight)
        DECLARE_HASH(pk_Bloom_Texture)
        DECLARE_HASH(pk_Bloom_LensDirtTex)

        DECLARE_HASH(pk_FilmGrain_Texture)

        DECLARE_HASH(pk_CC_LutTex)
        DECLARE_HASH(pk_Tonemap_LutTex)

        DECLARE_HASH(pk_DoF_FocalLength)
        DECLARE_HASH(pk_DoF_FNumber)
        DECLARE_HASH(pk_DoF_FilmHeight)
        DECLARE_HASH(pk_DoF_FocusSpeed)
        DECLARE_HASH(pk_DoF_MaximumCoC)
        DECLARE_HASH(pk_DoF_AutoFocusState)
        DECLARE_HASH(pk_DoF_ColorRead)
        DECLARE_HASH(pk_DoF_ColorWrite)
        DECLARE_HASH(pk_DoF_AlphaRead)
        DECLARE_HASH(pk_DoF_AlphaWrite)

        DECLARE_HASH(pk_Fog_Inject_Read)
        DECLARE_HASH(pk_Fog_Density_Read)
        DECLARE_HASH(pk_Fog_Scatter_Read)
        DECLARE_HASH(pk_Fog_Inject_Write)
        DECLARE_HASH(pk_Fog_Density_Write)
        DECLARE_HASH(pk_Fog_Scatter_Write)

        DECLARE_HASH(pk_Meshlet_Tasklets)
        DECLARE_HASH(pk_Meshlet_DispatchOffset)
        DECLARE_HASH(pk_Meshlet_Submeshes)
        DECLARE_HASH(pk_Meshlets)
        DECLARE_HASH(pk_Meshlet_Vertices)
        DECLARE_HASH(pk_Meshlet_Indices)

        DECLARE_HASH(pk_Gizmos_IndirectVertices)
        DECLARE_HASH(pk_Gizmos_IndirectArguments)
        DECLARE_HASH(pk_GUI_Vertices)
        DECLARE_HASH(pk_GUI_ScreenTransform)

        DECLARE_HASH(PK_LIGHT_PASS_DIRECTIONAL)
        DECLARE_HASH(PK_LIGHT_PASS_SPOT)
        DECLARE_HASH(PK_LIGHT_PASS_POINT)

        DECLARE_HASH(PK_INSTANCING_ENABLED)
        DECLARE_HASH(PK_META_PASS_GBUFFER)
        DECLARE_HASH(PK_META_PASS_GIVOXELIZE)

        NameID pk_Instancing_Transforms = NameID(PKAssets::PK_SHADER_INSTANCING_TRANSFORMS);
        NameID pk_Instancing_Indices = NameID(PKAssets::PK_SHADER_INSTANCING_INDICES);
        NameID pk_Instancing_Properties = NameID(PKAssets::PK_SHADER_INSTANCING_PROPERTIES);

        NameID STAT_GUI = NameID("GUI");
        NameID STAT_POSTFX = NameID("PostProcessing");
        NameID STAT_GI_RT = NameID("SceneGI.Raytrace");
        NameID STAT_GI_REPROJECT = NameID("SceneGI.Reproject");
        NameID STAT_GI_VOXELIZE = NameID("SceneGI.Voxelize");
        NameID STAT_GI_FILTER = NameID("SceneGI.Filter");
    };
}
