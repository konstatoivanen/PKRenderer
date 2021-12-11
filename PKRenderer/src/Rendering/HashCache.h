#pragma once
#include "Core/ISingleton.h"
#include "Core/IService.h"
#include "Utilities/StringHashID.h"

namespace PK::Rendering
{
    struct HashCache : public IService, public ISingleton<HashCache>
    {
        #define DEFINE_HASH_CACHE(name) unsigned int name = StringHashID::StringToID(#name); \
    
        DEFINE_HASH_CACHE(_MainTex)
        DEFINE_HASH_CACHE(pk_Time)
        DEFINE_HASH_CACHE(pk_SinTime)
        DEFINE_HASH_CACHE(pk_CosTime)
        DEFINE_HASH_CACHE(pk_DeltaTime)
        DEFINE_HASH_CACHE(pk_CursorParams)
        DEFINE_HASH_CACHE(pk_WorldSpaceCameraPos)
        DEFINE_HASH_CACHE(pk_ProjectionParams)
        DEFINE_HASH_CACHE(pk_ExpProjectionParams)
        DEFINE_HASH_CACHE(pk_ScreenParams)
        DEFINE_HASH_CACHE(pk_ShadowCascadeZSplits)
        DEFINE_HASH_CACHE(pk_MATRIX_M)
        DEFINE_HASH_CACHE(pk_MATRIX_I_M)
        DEFINE_HASH_CACHE(pk_MATRIX_V)
        DEFINE_HASH_CACHE(pk_MATRIX_I_V)
        DEFINE_HASH_CACHE(pk_MATRIX_P)
        DEFINE_HASH_CACHE(pk_MATRIX_I_P)
        DEFINE_HASH_CACHE(pk_MATRIX_VP)
        DEFINE_HASH_CACHE(pk_MATRIX_I_VP)
        DEFINE_HASH_CACHE(pk_MATRIX_L_VP)
        
        DEFINE_HASH_CACHE(pk_SceneOEM_HDR)
        DEFINE_HASH_CACHE(pk_SceneOEM_ST)
        DEFINE_HASH_CACHE(pk_SceneOEM_RVS)
        DEFINE_HASH_CACHE(pk_SceneOEM_Exposure)

        DEFINE_HASH_CACHE(pk_InstancingMatrices)
        DEFINE_HASH_CACHE(pk_InstancingPropertyIndices)
        DEFINE_HASH_CACHE(pk_InstancedProperties)
        DEFINE_HASH_CACHE(PK_ENABLE_INSTANCING)

        DEFINE_HASH_CACHE(pk_PerFrameConstants)
        DEFINE_HASH_CACHE(pk_GizmoVertices)
        DEFINE_HASH_CACHE(pk_DebugDrawIndex)
        
        DEFINE_HASH_CACHE(pk_ScreenDepth)
        DEFINE_HASH_CACHE(pk_ScreenNormals)
        DEFINE_HASH_CACHE(pk_ScreenOcclusion)
        DEFINE_HASH_CACHE(pk_ShadowmapAtlas)
        DEFINE_HASH_CACHE(pk_LightCookies)
        DEFINE_HASH_CACHE(pk_Bluenoise256)

        DEFINE_HASH_CACHE(pk_Lights)
        DEFINE_HASH_CACHE(pk_LightMatrices)
        DEFINE_HASH_CACHE(pk_LightDirections)
        DEFINE_HASH_CACHE(pk_LightCount)
        DEFINE_HASH_CACHE(pk_GlobalLightsList)
        DEFINE_HASH_CACHE(pk_LightTiles)
        DEFINE_HASH_CACHE(pk_TileMaxDepths)
        DEFINE_HASH_CACHE(pk_GlobalListListIndex)

        DEFINE_HASH_CACHE(pk_MinLogLuminance)
        DEFINE_HASH_CACHE(pk_InvLogLuminanceRange)
        DEFINE_HASH_CACHE(pk_LogLuminanceRange)
        DEFINE_HASH_CACHE(pk_TargetExposure)
        DEFINE_HASH_CACHE(pk_AutoExposureSpeed)
        DEFINE_HASH_CACHE(pk_BloomIntensity)
        DEFINE_HASH_CACHE(pk_BloomDirtIntensity)

        DEFINE_HASH_CACHE(pk_Vibrance)
        DEFINE_HASH_CACHE(pk_VignetteGrain)
        DEFINE_HASH_CACHE(pk_WhiteBalance)
        DEFINE_HASH_CACHE(pk_Lift)
        DEFINE_HASH_CACHE(pk_Gamma)
        DEFINE_HASH_CACHE(pk_Gain)
        DEFINE_HASH_CACHE(pk_ContrastGainGammaContribution)
        DEFINE_HASH_CACHE(pk_HSV)
        DEFINE_HASH_CACHE(pk_ChannelMixerRed)
        DEFINE_HASH_CACHE(pk_ChannelMixerGreen)
        DEFINE_HASH_CACHE(pk_ChannelMixerBlue)
        DEFINE_HASH_CACHE(pk_FilmGrainTex)
        DEFINE_HASH_CACHE(pk_BloomLensDirtTex)
        DEFINE_HASH_CACHE(pk_HDRScreenTex)
        DEFINE_HASH_CACHE(pk_TonemappingParams)
        DEFINE_HASH_CACHE(pk_Histogram)

        DEFINE_HASH_CACHE(pk_FocalLength)
        DEFINE_HASH_CACHE(pk_FNumber)
        DEFINE_HASH_CACHE(pk_FilmHeight)
        DEFINE_HASH_CACHE(pk_FocusSpeed)
        DEFINE_HASH_CACHE(pk_MaximumCoC)
        DEFINE_HASH_CACHE(pk_DofParams)
        DEFINE_HASH_CACHE(pk_AutoFocusParams)

        DEFINE_HASH_CACHE(pk_Volume_WindDir)
        DEFINE_HASH_CACHE(pk_Volume_ConstantFog)
        DEFINE_HASH_CACHE(pk_Volume_HeightFogExponent)
        DEFINE_HASH_CACHE(pk_Volume_HeightFogOffset)
        DEFINE_HASH_CACHE(pk_Volume_HeightFogAmount)
        DEFINE_HASH_CACHE(pk_Volume_Density)
        DEFINE_HASH_CACHE(pk_Volume_Intensity)
        DEFINE_HASH_CACHE(pk_Volume_Anisotropy)
        DEFINE_HASH_CACHE(pk_Volume_NoiseFogAmount)
        DEFINE_HASH_CACHE(pk_Volume_NoiseFogScale)
        DEFINE_HASH_CACHE(pk_Volume_WindSpeed)
        DEFINE_HASH_CACHE(pk_Volume_ScatterRead)
        DEFINE_HASH_CACHE(pk_Volume_InjectRead)
        DEFINE_HASH_CACHE(pk_Volume_Inject)
        DEFINE_HASH_CACHE(pk_Volume_Scatter)
        DEFINE_HASH_CACHE(pk_VolumeResources)
        DEFINE_HASH_CACHE(pk_VolumeMaxDepths)

        DEFINE_HASH_CACHE(_BloomPassParams)
        DEFINE_HASH_CACHE(_AOPassParams)
        DEFINE_HASH_CACHE(_AOParams)
        DEFINE_HASH_CACHE(_Color)

        DEFINE_HASH_CACHE(_ShadowmapBatchCube)
        DEFINE_HASH_CACHE(_ShadowmapBatch0)
        DEFINE_HASH_CACHE(_ShadowmapBatch1)

        #undef DEFINE_HASH_CACHE
    };
}