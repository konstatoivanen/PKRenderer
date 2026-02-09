
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_program SHADER_STAGE_COMPUTE PostFilterCs

#define PK_GI_LOAD_LVL 0
#define PK_GI_STORE_LVL 1

#include "includes/SceneGIFiltering.glsl"
#include "includes/ComputeQuadSwap.glsl"

[pk_numthreads(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 1u)]
void PostFilterCs()
{
    const int2 coord_base = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(coord_base));
    const int2 coord_n = GI_ExpandCheckerboardCoord(uint2(coord_base), 1u);
    const int2 coord_h = int2(coord_base.x + pk_ScreenSize.x / 2, coord_base.y);

    float depth = SampleMinZ(coord, 0);
    float4 normal_roughness = SampleViewNormalRoughness(coord);
    float3 normal = normal_roughness.xyz;
    float roughness = normal_roughness.w;
    float3 view_pos = CoordToViewPos(coord, depth);
    float3 view_dir = normalize(view_pos);

    const bool is_scene = Test_DepthIsScene(depth);

    GIDiff diff = GI_Load_Diff(coord_base);
    GISpec spec = GI_Load_Spec(coord_base);

    // Filter Diff
    {
        GIDiff history = diff;

        {
            float variance = 0.0f;
            GI_SF_DIFF_VARIANCE(coord, depth, diff, variance)

            const float2 radius_scale = GI_GetDiskFilterRadiusAndScale(depth, variance, diff.ao, diff.history);
            const float scale = radius_scale.y;
            const float radius = radius_scale.x * (scale + 1e-4f);
            const bool skip = diff.history > 30.0f || scale < 0.05f;
            const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
            GI_SF_DISK_DIFF(normal, depth, view_dir, view_pos, diff.history, step, skip, radius, diff)
        }

        const float alpha = GI_Alpha(diff);

        float luma_max;
        GI_SUBGROUP_ANTIFIREFLY_MAXLUMA(is_scene, diff, history, alpha, 1.0f, luma_max)

        history = GI_ClampLuma(history, luma_max);
        history.history += 1.0f;
        diff.ao = lerp(history.ao, 0.5f + diff.ao * 0.5f, alpha);
        GI_Store_Diff(coord, history);
    }

        // Filter Spec
#if PK_GI_APPROX_ROUGH_SPEC == 1
    [[branch]]
    if (roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        GISpec history = spec;

        /*
        // @TODO Calculate different radius for this as diffuse variance is hardly usable & roughness is more of a relevant factor.
        const float2 radiusAndScale = GI_GetDiskFilterRadiusAndScale(depth, 0.0f, spec.ao, spec.history);
        const float scale = radiusAndScale.y * sqrt(roughness);
        const float radius = radiusAndScale.x * (scale + 1e-4f);
        const bool skip = scale < 0.05f;
        const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
        GI_SF_DISK_SPEC(normal, depth, roughness, view_dir, view_pos, spec.history, step, skip, radius, spec)
        */

        const float alpha = pow2(GI_Alpha(history));
        history = GI_ClampLuma(history, GI_MaxLuma(spec, alpha));
        history.history += 1.0f;
        spec.ao = lerp(history.ao, 0.5f + spec.ao * 0.5f, alpha);
        GI_Store_Spec(coord, history);
    }

    GI_Store_Resolved(coord, diff, spec);

#if defined(PK_GI_CHECKERBOARD_TRACE)
    // Checkerboard resolve.
    {
        const float n_depth = SampleMinZ(coord_n, 0);
        const float4 n_normal_roughness = SampleViewNormalRoughness(coord_n);
        const float3 n_normal = n_normal_roughness.xyz;
        const float n_roughness = n_normal_roughness.w;
        const float2 k_R = GI_GetRoughnessWeightParams(n_roughness);

        GIDiff n_diff = PK_GI_DIFF_ZERO;
        GISpec n_spec = PK_GI_SPEC_ZERO;
        float weight_sum_diff = 0.0f;
        float weight_sum_spec = 0.0f;

        // Weight current sample
        {
            const float w_diff = max(0.0f, dot(n_normal, normal)) * exp(-abs(n_depth - depth));
            const float w_spec = w_diff * exp(-abs(roughness * k_R.x + k_R.y));
            n_diff = GI_Sum_NoHistory(n_diff, diff, w_diff);
            n_spec = GI_Sum_NoHistory(n_spec, spec, w_spec);
            weight_sum_diff += w_diff;
            weight_sum_spec += w_spec;
        }

        // Shuffle with chb neighbour.
        {
            const uint swapId = QuadSwapIdVertical8x8(gl_SubgroupInvocationID);
            depth = subgroupShuffle(depth, swapId);
            normal = subgroupShuffle(normal, swapId);
            roughness = subgroupShuffle(roughness, swapId);
            diff.sh.Y = subgroupShuffle(diff.sh.Y, swapId);
            diff.sh.A = subgroupShuffle(diff.sh.A, swapId);
            diff.ao = subgroupShuffle(diff.ao, swapId);
            spec.radiance = subgroupShuffle(spec.radiance, swapId);
            spec.ao = subgroupShuffle(spec.ao, swapId);
        }


        // Weight neighbour sample
        {
            const float w_diff = max(0.0f, dot(n_normal, normal)) * exp(-abs(n_depth - depth));
            const float w_spec = w_diff * exp(-abs(roughness * k_R.x + k_R.y));
            n_diff = GI_Sum_NoHistory(n_diff, diff, w_diff);
            n_spec = GI_Sum_NoHistory(n_spec, spec, w_spec);
            weight_sum_diff += w_diff;
            weight_sum_spec += w_spec;
        }

        // Store diff
        {
            GIDiff nh_diff = GI_Load_Diff(coord_h);

            const float alpha = GI_Alpha(nh_diff) * saturate(10.0f * weight_sum_diff);
            n_diff = GI_Mul_NoHistory(n_diff, Test_NaN_EPS6(weight_sum_diff) ? 0.0f : 1.0f / weight_sum_diff);
            n_diff = GI_Interpolate(nh_diff, n_diff, alpha);
            GI_Store_Diff(coord_n, GI_Interpolate(nh_diff, n_diff, alpha));
        }

        // Store spec
#if PK_GI_APPROX_ROUGH_SPEC == 1
        [[branch]]
        if (n_roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
        {
            GISpec nh_spec = GI_Load_Spec(coord_h);

            const float alpha = GI_Alpha(nh_spec) * saturate(10.0f * weight_sum_spec);
            n_spec = GI_Mul_NoHistory(n_spec, Test_NaN_EPS6(weight_sum_spec) ? 0.0f : 1.0f / weight_sum_spec);
            n_spec = GI_Interpolate(nh_spec, n_spec, alpha);
            GI_Store_Spec(coord_n, GI_Interpolate(nh_spec, n_spec, alpha));
        }

        GI_Store_Resolved(coord_n, n_diff, n_spec);
    }
#endif
}