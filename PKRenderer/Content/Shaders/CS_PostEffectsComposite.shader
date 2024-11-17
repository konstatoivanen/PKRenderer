
#extension GL_KHR_shader_subgroup_shuffle : require
#pragma pk_multi_compile FX_APPLY_ALL FX_APPLY_MASK FX_APPLY_DEBUG
#pragma pk_program SHADER_STAGE_COMPUTE main

#include "includes/PostFXColorGrading.glsl"
#include "includes/PostFXFilmGrain.glsl"
#include "includes/PostFXBloom.glsl"
#include "includes/PostFXAutoExposure.glsl"
#include "includes/Common.glsl"
#include "includes/ComputeQuadSwap.glsl"
#include "includes/Noise.glsl"

#if defined(FX_APPLY_DEBUG)
#include "includes/GBuffers.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/SceneGIVX.glsl"
#endif

PK_DECLARE_SET_DRAW uniform image2D pk_Image;

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
#if 1
        color = Tonemap_AgX(color, exposure);
#else
        // Applying a bit of desaturation to reduce high intensity value color blowout
        color = Saturate_BT2100(color, 0.96f);
        color = Tonemap_Uchimura(color, exposure);
        color = Saturate_BT2100(color, 0.93f);
#endif
    }

    IF_FX_FEATURE_ENABLED(FX_FEAT_FILMGRAIN)
    {
        color = FilmGrain(float2(coord), color, exposure);
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

    // Dither
    {
        float noise = 0.0f;
        noise = InterleavedGradientNoise(coord, pk_FrameIndex.x);
        noise = noise * 2.0f - 1.0f;
        noise = sign(noise) - sign(noise) * sqrt(saturate(1.0f - abs(noise)));
        noise = noise / 255.0f;
        color += noise.xxx;
    }

    // Debug previews
#if defined(FX_APPLY_DEBUG)
    if (uv.x > 0.5 || !IS_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_HALFSCREEN))
    {
        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_ZOOM)
        {
            uv = (uv - 0.5f) * 0.05f + 0.5f;
        }

        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_GI_DIFF)
        {
            color = SH_ToDiffuse(GI_Load_Resolved(uv).diffSH, SampleWorldNormal(uv)) * exposure;
        }

        IF_FX_FEATURE_ENABLED(FX_FEAT_DEBUG_GI_SPEC)
        {
            color = GI_Load_Resolved(uv).spec * exposure;
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
