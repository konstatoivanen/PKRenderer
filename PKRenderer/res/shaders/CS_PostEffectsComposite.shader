#extension GL_KHR_shader_subgroup_shuffle : require

#pragma PROGRAM_COMPUTE
#include includes/PostFXColorGrading.glsl
#include includes/PostFXFilmGrain.glsl
#include includes/PostFXBloom.glsl
#include includes/PostFXAutoExposure.glsl
#include includes/Common.glsl

#include includes/ComputeQuadSwap.glsl

#multi_compile FX_APPLY_ALL FX_APPLY_MASK FX_APPLY_DEBUG

#if defined(FX_APPLY_DEBUG)
#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SceneGIVX.glsl
#endif

layout(rgba16f, set = PK_SET_DRAW) uniform image2D pk_Image;

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 size = imageSize(pk_Image).xy;
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    float2 uv = float2(coord + 0.5f.xx) / float2(size);

    float3 color = imageLoad(pk_Image, coord).rgb;

    // Remove nans
    color = -min(-color, 0.0f.xxx);

    float exposure = GetAutoExposure();
    
    IF_FX_FEATURE_ENABLED(FX_FEAT_VIGNETTE)
    {
        exposure *= Vignette(uv);
    }

    IF_FX_FEATURE_ENABLED(FX_FEAT_BLOOM)
    {
        color = Bloom(color, uv);
    }

    IF_FX_FEATURE_ENABLED(FX_FEAT_TONEMAP)
    {
        // Applying a bit of desaturation to reduce high intensity value color blowout
        color = Saturate_BT2100(color, 0.96f);
        color = Tonemap_Uchimura(color, exposure);
        color = Saturate_BT2100(color, 0.93f);
    }

    IF_FX_FEATURE_ENABLED(FX_FEAT_FILMGRAIN)
    {
        color = FilmGrain(color, float2(coord));
    }

    color = LinearToGamma(color);

    // Prefer lut color grading in apply all mode.
    #if !defined(FX_APPLY_ALL)
    IF_FX_FEATURE_ENABLED(FX_FEAT_COLORGRADING)
    {
        color = ApplyColorGrading(color);
    }
    #endif

    IF_FX_FEATURE_ENABLED(FX_FEAT_LUTCOLORGRADING)
    {
        color = ApplyLutColorGrading(color);
    }

    // Debug previews
    #if defined(FX_APPLY_DEBUG)
    if (uv.x > 0.5 && IS_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_HALFSCREEN))
    {
        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_ZOOM)
        {
            uv = (uv - 0.5f) * 0.05f + 0.5f;
        }

        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_GI_DIFF)
        {
            color = GI_Load_Resolved_Diff(uv) * exposure;
        }
        
        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_GI_SPEC)
        {
            color = GI_Load_Resolved_Spec(uv) * exposure;
        }

        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_GI_VX)
        {   
            float depth = SampleViewDepth(uv);
            float4 voxel = GI_Load_Voxel(UVToWorldPos(uv, depth), 1.5f);
            voxel.rgb *= safePositiveRcp(voxel.a);
            color = voxel.rgb * exposure;
        }

        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_NORMAL)
        {
            color = SampleWorldNormal(uv) * 0.5f + 0.5f;
        }

        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_ROUGHNESS)
        {
            color = SampleRoughness(uv).xxx;
        }
    }
    #endif

    imageStore(pk_Image, coord, float4(color, 1.0f));
}