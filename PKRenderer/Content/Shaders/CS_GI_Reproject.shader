
#pragma pk_multi_compile _ PK_GI_SPEC_VIRT_REPROJECT
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_program SHADER_STAGE_COMPUTE ReprojectCs

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include "includes/SceneGIFiltering.glsl"
#include "includes/ComputeQuadSwap.glsl"

float GI_GetAntilag_SubgroupStretch(float2 reproject_coord)
{
    const uint swapId = QuadSwapIdDiagonal16x2(gl_SubgroupInvocationID);
    const float2 reproject_coord_n = subgroupShuffle(reproject_coord, swapId);
    const float2 coord_diff = abs(reproject_coord_n - reproject_coord);
    const float  min_diff = min(coord_diff.x, coord_diff.y);
    return saturate(pow4(min_diff) / 1.0f);
}

[pk_numthreads(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 1u)]
void ReprojectCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);

#if defined(PK_GI_CHECKERBOARD_TRACE)
    const int2 coord_store = int2
    (
        coord.x / 2 + int(Checkerboard(coord, pk_FrameIndex.y) * (pk_ScreenSize.x / 2)),
        coord.y
    );
#else
    const int2 coord_store = coord;
#endif

    const float depth = PK_GI_SAMPLE_DEPTH(coord);
    const bool is_scene = Test_DepthIsScene(depth);
    uint4 packed_diff = uint4(0u);
    uint2 packed_spec = uint2(0u);

    // Far clip or new backbuffer
    [[branch]]
    if (pk_FrameIndex.y != 0u && subgroupAny(is_scene))
    {
        GIDiff diff = PK_GI_DIFF_ZERO;
        GISpec spec = PK_GI_SPEC_ZERO;
        GISpec spec_virt = PK_GI_SPEC_ZERO;
        float wsum_diff = 0.0f;
        float wsum_spec = 0.0f;
        float wsum_spec_virt = 0.0f;
        float antilag_spec = 1.0f;
        float antilag_diff = 1.0f;
        bool discard_spec = false;

        // Filters
        {
            const float4 normal_roughness = SampleViewNormalRoughness(coord);
            const float3 normal = normal_roughness.xyz;
            const float roughness = normal_roughness.w;
            const float depth_bias = lerp(0.1f, 0.01f, -normal.z);
            const float3 view_pos = CoordToViewPos(coord, depth);
            const float3 view_dir = normalize(view_pos);
            const float nv = dot(normal, -view_dir);
            const float parallax = GI_GetParallax(view_dir, normalize(view_pos - pk_ViewSpaceCameraDelta.xyz));

#if PK_GI_APPROX_ROUGH_SPEC == 1
            discard_spec = roughness >= PK_GI_MAX_ROUGH_SPEC;
#endif

            // Reconstruct diff & naive spec
            {
                const float2 k_R = GI_GetRoughnessWeightParams(roughness);
                const float2 s_fcoord = GI_ViewToPrevScreenUv(view_pos);
                const int2   s_coord = int2(s_fcoord);
                const float4 s_depths = PK_GI_GATHER_PREV_DEPTH((s_coord + 0.5f.xx) * pk_ScreenParams.zw).wzxy;

                // Drop samples that elongate single pixels (Mitigates strech marks).
                antilag_diff *= GI_GetAntilag_SubgroupStretch(s_fcoord);

                float4 weights = GI_GetBilinearWeights(s_fcoord - s_coord);
                weights *= exp(-abs(depth.xxxx - s_depths));
                weights *= SafePositiveRcp(dot(weights, 1.0f.xxxx));
                weights *= float4(Test_DepthReproject(depth.xxxx, s_depths, depth_bias.xxxx));
                weights *= float4(Test_DepthIsScene(s_depths));
                weights *= float(Test_InUv(s_fcoord * pk_ScreenParams.zw));

                [[loop]]
                for (uint i = 0u; i < 4; ++i)
                {
                    const int2 xy = s_coord + int2(i % 2u, i / 2u);
                    const float4 s_nr = SamplePreviousViewNormalRoughness(xy);
                    const float w_n = pow5(dot(normal, s_nr.xyz));
                    float w_diff = weights[i] * w_n * float(w_n > 0.05f);
                    float w_spec = weights[i] * w_n * exp(-abs(s_nr.w * k_R.x + k_R.y));
                    w_diff = lerp(0.0f, w_diff, !Test_NaN_EPS6(w_diff));
                    w_spec = lerp(0.0f, w_spec, !Test_NaN_EPS6(w_spec));

                    // Using spec weight to save registers.
                    diff = GI_Sum(diff, GI_Load_Diff(xy), w_spec);
                    wsum_diff += w_spec;

                    // Spec
                    [[branch]]
                    if (!discard_spec && (PK_GI_APPROX_ROUGH_SPEC == 0 || s_nr.w < PK_GI_MAX_ROUGH_SPEC))
                    {
                        spec = GI_Sum(spec, GI_Load_Spec(xy), w_spec);
                        wsum_spec += w_spec;
                    }
                }
            }

            // Reduce diff antilag on poor reproject.
            antilag_diff *= lerp(0.1f, 1.0f, saturate(wsum_diff));
            antilag_spec = GI_GetAntilagSpecular(roughness, nv, parallax);

#if defined(PK_GI_SPEC_VIRT_REPROJECT)
            [[branch]]
            if (!Test_EPS6(wsum_spec) && !discard_spec)
            {
                const float  s_vdist = (spec.ao / wsum_spec) * PK_GI_RAY_TMAX * Futil_SpecularDominantFactor(nv, roughness);
                const float2 k_R = GI_GetRoughnessWeightParams(roughness);
                const float2 s_fcoord = GI_ViewToPrevScreenUv(view_pos + view_dir * s_vdist);
                const int2   s_coord = int2(s_fcoord);
                const float4 s_depths = PK_GI_GATHER_PREV_DEPTH((s_coord + 0.5f.xx) * pk_ScreenParams.zw).wzxy;

                float4 weights = GI_GetBilinearWeights(s_fcoord - s_coord);
                weights *= 1.0f.xxxx / (1e-4f + abs(depth.xxxx - s_depths));
                weights *= float4(Test_DepthIsScene(s_depths));
                weights *= float(Test_InUv(s_fcoord * pk_ScreenParams.zw));

                [[loop]]
                for (uint i = 0u; i < 4; ++i)
                {
                    const int2 xy = s_coord + int2(i % 2u, i / 2u);
                    const float4 s_nr = SamplePreviousViewNormalRoughness(xy);
                    float w = weights[i];
                    w *= pow(saturate(dot(normal, s_nr.xyz)), 256.0f);
                    w *= exp(-abs(s_nr.w * k_R.x + k_R.y));
                    w = lerp(0.0f, w, !Test_NaN_EPS6(w));
#if PK_GI_APPROX_ROUGH_SPEC == 1
                    w = lerp(0.0f, w, s_nr.w < PK_GI_MAX_ROUGH_SPEC);
#endif
                    spec_virt = GI_Sum(spec_virt, GI_Load_Spec(xy), w);
                    wsum_spec_virt += w;
                }
            }
#endif
        }

        // Normalization
        {
            diff.history = min(diff.history / wsum_diff, PK_GI_DIFF_MAX_HISTORY * antilag_diff);
            spec.history = min(spec.history / wsum_spec, PK_GI_SPEC_MAX_HISTORY * antilag_spec);
            diff = GI_Mul_NoHistory(diff, 1.0f / wsum_diff);
            spec = GI_Mul_NoHistory(spec, 1.0f / wsum_spec);

            // Get min of virtual reprojected spec & naive spec to eliminate ghosting.
            if (!Test_NaN_EPS6(wsum_spec_virt))
            {
                spec_virt.history = clamp(spec_virt.history / wsum_spec_virt, 0.0f, PK_GI_SPEC_MAX_HISTORY * antilag_spec);
                spec_virt = GI_Mul_NoHistory(spec_virt, 1.0f / wsum_spec_virt);
                spec.history = min(spec_virt.history, spec.history);
                spec.radiance = min(spec.radiance, spec_virt.radiance);
                spec.ao = min(spec.ao, spec_virt.ao);
            }
        }

        const bool invalid_diff = !is_scene || Test_NaN_EPS6(wsum_diff);
        const bool invalid_spec = !is_scene || Test_NaN_EPS6(wsum_spec);
        packed_diff = invalid_diff ? uint4(0) : GI_Pack_Diff(diff);
        packed_spec = invalid_spec ? uint2(0) : GI_Pack_Spec(spec);
    }

    GI_Store_Packed_Diff(coord_store, packed_diff);
    GI_Store_Packed_Spec(coord_store, packed_spec);
}
