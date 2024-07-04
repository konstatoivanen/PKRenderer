#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#PK_MultiCompile _ PK_GI_CHECKERBOARD_TRACE

#pragma PROGRAM_COMPUTE
#define PK_GI_LOAD_LVL 0
#define PK_GI_STORE_LVL 1

#include "includes/SceneGIFiltering.glsl"
#include "includes/ComputeQuadSwap.glsl"

#define SUBGROUP_ANTIFIREFLY_MAXLUMA(condition, current, history, alpha, lumaMax)               \
{                                                                                               \
    const uint4 threadMask = subgroupBallot(condition);                                         \
    const uint threadCount = max(1u, subgroupBallotBitCount(threadMask)) - 1u;                  \
                                                                                                \
    const float2 moments = make_moments(GI_Luminance(current));                                 \
    const float2 momentsWave = (subgroupAdd(moments) - moments) / threadCount;                  \
                                                                                                \
    const float variance = pow(abs(momentsWave.y - pow2(momentsWave.x)), 0.25f);                \
    lumaMax = lerp(GI_Luminance(history), momentsWave.x, alpha) + variance;                     \
}                                                                                               \

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const int2 ncoord = GI_ExpandCheckerboardCoord(uint2(baseCoord), 1u);
    const int2 hcoord = int2(baseCoord.x + pk_ScreenSize.x / 2, baseCoord.y);

    float depth = SampleMinZ(coord, 0);
    float4 normalRoughness = SampleViewNormalRoughness(coord);
    float3 normal = normalRoughness.xyz;
    float roughness = normalRoughness.w;
    float3 viewpos = CoordToViewPos(coord, depth);
    float3 viewdir = normalize(viewpos);

    const bool isScene = Test_DepthFar(depth);

    GIDiff diff = GI_Load_Diff(baseCoord);
    GISpec spec = GI_Load_Spec(baseCoord);
    GISpec ra_spec = PK_GI_SPEC_ZERO;

    // Filter Diff
    {
        GIDiff history = diff;

        {
            float variance = 0.0f;
            GI_SF_DIFF_VARIANCE(coord, depth, diff, variance)

            const float2 radiusAndScale = GI_GetDiskFilterRadiusAndScale(depth, variance, diff.ao, diff.history);
            const float scale = radiusAndScale.y;
            const float radius = radiusAndScale.x * (scale + 1e-4f);
            const bool skip = diff.history > 30.0f || scale < 0.05f;
            const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
            GI_SF_DISK_DIFF(normal, depth, viewdir, viewpos, diff.history, step, skip, radius, diff)
        }

        const float alpha = GI_Alpha(diff);

        float lumaMax;
        SUBGROUP_ANTIFIREFLY_MAXLUMA(isScene, diff, history, alpha, lumaMax)

        history = GI_ClampLuma(history, lumaMax);
        history.history += 1.0f;

        diff.ao = lerp(history.ao, 0.5f + diff.ao * 0.5f, alpha);

        GI_Store_Diff(coord, history);
        GI_Store_Resolved_Diff(coord, ViewToWorldVec(normal), diff);

#if PK_GI_APPROX_ROUGH_SPEC == 1
        ra_spec = GI_ShadeRoughSpecular(normal, viewdir, roughness, diff);
#endif
    }

    // Filter Spec
    {
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
            GI_SF_DISK_SPEC(normal, depth, roughness, viewdir, viewpos, spec.history, step, skip, radius, spec)
            */

            const float alpha = pow2(GI_Alpha(history));
            history = GI_ClampLuma(history, GI_MaxLuma(spec, alpha));
            history.history += 1.0f;

            spec.ao = lerp(history.ao, 0.5f + spec.ao * 0.5f, alpha);

            GI_Store_Spec(coord, history);
        }

#if PK_GI_APPROX_ROUGH_SPEC == 1
        spec = GI_Interpolate(spec, ra_spec, GI_RoughSpecWeight(roughness));
#endif

        GI_Store_Resolved_Spec(coord, spec);
    }

#if defined(PK_GI_CHECKERBOARD_TRACE)
    {
        const float n_depth = SampleMinZ(ncoord, 0);
        const float4 n_normalRoughness = SampleViewNormalRoughness(ncoord);
        const float3 n_normal = n_normalRoughness.xyz;
        const float n_roughness = n_normalRoughness.w;
        const float2 k_R = GI_GetRoughnessWeightParams(n_roughness);

        GIDiff n_diff = PK_GI_DIFF_ZERO;
        GISpec n_spec = PK_GI_SPEC_ZERO;
        float wSumDiff = 0.0f;
        float wSumSpec = 0.0f;

        // Weight current sample
        {
            const float w_diff = max(0.0f, dot(n_normal, normal)) * exp(-abs(n_depth - depth));
            const float w_spec = w_diff * exp(-abs(roughness * k_R.x + k_R.y));
            n_diff = GI_Sum_NoHistory(n_diff, diff, w_diff);
            n_spec = GI_Sum_NoHistory(n_spec, spec, w_spec);
            wSumDiff += w_diff;
            wSumSpec += w_spec;
        }

        // Shuffle with chb neighbour.
        {
            uint swapId = QuadSwapIdVertical8x8(gl_SubgroupInvocationID);
            depth = subgroupShuffle(depth, swapId);
            normal = subgroupShuffle(normal, swapId);
            roughness = subgroupShuffle(roughness, swapId);
            diff.sh.Y = subgroupShuffle(diff.sh.Y, swapId);
            diff.sh.CoCg = subgroupShuffle(diff.sh.CoCg, swapId);
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
            wSumDiff += w_diff;
            wSumSpec += w_spec;
        }

        // Store diff
        {
            GIDiff nh_diff = GI_Load_Diff(hcoord);

            const float alpha = GI_Alpha(nh_diff) * saturate(10.0f * wSumDiff);
            n_diff = GI_Mul_NoHistory(n_diff, Test_NaN_EPS6(wSumDiff) ? 0.0f : 1.0f / wSumDiff);
            n_diff = GI_Interpolate(nh_diff, n_diff, alpha);
            GI_Store_Diff(ncoord, GI_Interpolate(nh_diff, n_diff, alpha));
            GI_Store_Resolved_Diff(ncoord, ViewToWorldVec(n_normal), n_diff);
        }

        // Store spec
        {
#if PK_GI_APPROX_ROUGH_SPEC == 1
            [[branch]]
            if (n_roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
            {
                GISpec nh_spec = GI_Load_Spec(hcoord);

                const float alpha = GI_Alpha(nh_spec) * saturate(10.0f * wSumSpec);
                n_spec = GI_Mul_NoHistory(n_spec, Test_NaN_EPS6(wSumSpec) ? 0.0f : 1.0f / wSumSpec);
                n_spec = GI_Interpolate(nh_spec, n_spec, alpha);
                GI_Store_Spec(ncoord, GI_Interpolate(nh_spec, n_spec, alpha));
            }

#if PK_GI_APPROX_ROUGH_SPEC == 1
            {
                const float3 viewdir = normalize(CoordToViewPos(ncoord, n_depth));
                ra_spec = GI_ShadeRoughSpecular(n_normal, viewdir, n_roughness, n_diff);
                n_spec = GI_Interpolate(n_spec, ra_spec, GI_RoughSpecWeight(n_roughness));
            }
#endif

            GI_Store_Resolved_Spec(ncoord, n_spec);
        }
    }
#endif
}