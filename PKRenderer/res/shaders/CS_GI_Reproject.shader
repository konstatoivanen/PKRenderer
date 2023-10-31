#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_SPEC_VIRT_REPROJECT
#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include includes/SceneGIFiltering.glsl

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    #if defined(PK_GI_CHECKERBOARD_TRACE)
    const int2 storeCoord = int2
    (
        coord.x / 2 + int(GI_GetCheckerboardOffset(coord, pk_FrameIndex.y) * (pk_ScreenSize.x / 2)),
        coord.y
    );
    #else
    const int2 storeCoord = coord;
    #endif

    const float depth = PK_GI_SAMPLE_DEPTH(coord);
    uint4 packedDiff = uint4(0u);
    uint2 packedSpec = uint2(0u);

    // Far clip or new backbuffer
    [[branch]]
    if (pk_FrameIndex.y != 0u && Test_DepthFar(depth))
    {
        GIDiff diff = PK_GI_DIFF_ZERO;
        GISpec spec = PK_GI_SPEC_ZERO;
        GISpec specVirtual = PK_GI_SPEC_ZERO;
        float wSumDiff = 0.0f;
        float wSumSpec = 0.0f;
        float wSumVSpec = 0.0f;
        float antilagSpec = 1.0f;
        float antilagDiff = 1.0f;
        bool discardSpec = false;

        // Filters
        {
            const float4 normalroughness = SampleViewNormalRoughness(coord);
            const float3 normal = normalroughness.xyz;
            const float roughness = normalroughness.w;
            const float depthBias = lerp(0.1f, 0.01f, -normal.z);
            const float3 viewpos = CoordToViewPos(coord, depth);
            const float3 viewdir = normalize(viewpos);
            const float nv = dot(normal, -viewdir);
            const float parallax = GI_GetParallax(viewdir, normalize(viewpos - pk_ViewSpaceCameraDelta.xyz));
            
            #if PK_GI_APPROX_ROUGH_SPEC == 1
            discardSpec = roughness >= PK_GI_MAX_ROUGH_SPEC;
            #endif

            // Reconstruct diff & naive spec
            {
                const float2 k_R = GI_GetRoughnessWeightParams(roughness);
                const float2 s_screenuv = GI_ViewToPrevScreenUV(viewpos);
                const int2   s_coord = int2(s_screenuv);
                const float4 s_depths = PK_GI_GATHER_PREV_DEPTH((s_coord + 0.5f.xx) * pk_ScreenParams.zw).wzxy;
                
                float4 weights = GI_GetBilinearWeights(s_screenuv - s_coord);
                weights *= exp(-abs(depth.xxxx - s_depths));
                weights *= safePositiveRcp(dot(weights, 1.0f.xxxx));
                weights *= float4(Test_DepthReproject(depth.xxxx, s_depths, depthBias.xxxx));
                weights *= float4(Test_DepthFar(s_depths));
                weights *= float(Test_InUV(s_screenuv * pk_ScreenParams.zw));
                
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

                    diff = GI_Sum(diff, GI_Load_Diff(xy), w_spec);
                    wSumDiff += w_spec;

                    // Spec
                    [[branch]]
                    if (!discardSpec && (PK_GI_APPROX_ROUGH_SPEC == 0 || s_nr.w < PK_GI_MAX_ROUGH_SPEC))
                    {
                        spec = GI_Sum(spec, GI_Load_Spec(xy), w_spec);
                        wSumSpec += w_spec;
                    }
                }
            }

            // Reduce diff antilag on poor reproject.
            antilagDiff = lerp(0.1f, 1.0f, saturate(wSumDiff));
            antilagSpec = GI_GetAntilagSpecular(roughness, nv, parallax);

            #if defined(PK_GI_SPEC_VIRT_REPROJECT)
            [[branch]]
            if (!Test_EPS6(wSumSpec) && !discardSpec)
            {
                const float  s_vdist = (spec.ao / wSumSpec) * PK_GI_RAY_TMAX * GI_GetSpecularDominantFactor(nv, roughness);
                const float2 k_R = GI_GetRoughnessWeightParams(roughness);
                const float2 s_screenuv = GI_ViewToPrevScreenUV(viewpos + viewdir * s_vdist);
                const int2   s_coord = int2(s_screenuv);
                const float4 s_depths = PK_GI_GATHER_PREV_DEPTH((s_coord + 0.5f.xx) * pk_ScreenParams.zw).wzxy;
                
                float4 weights = GI_GetBilinearWeights(s_screenuv - s_coord);
                weights *= 1.0f.xxxx / (1e-4f + abs(depth.xxxx - s_depths));
                weights *= float4(Test_DepthFar(s_depths));
                weights *= float(Test_InUV(s_screenuv * pk_ScreenParams.zw));
                
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
                    specVirtual = GI_Sum(specVirtual, GI_Load_Spec(xy), w);
                    wSumVSpec += w;
                }
            }
            #endif
        }

        // Normalization
        {
            diff.history = min(diff.history / wSumDiff, PK_GI_DIFF_MAX_HISTORY * antilagDiff);
            spec.history = min(spec.history / wSumSpec, PK_GI_SPEC_MAX_HISTORY * antilagSpec);
            diff = GI_Mul_NoHistory(diff, 1.0f / wSumDiff);
            spec = GI_Mul_NoHistory(spec, 1.0f / wSumSpec);
            
            // Get min of virtual reprojected spec & naive spec to eliminate ghosting.
            if (!Test_NaN_EPS6(wSumVSpec))
            {
                specVirtual.history = clamp(specVirtual.history / wSumVSpec, 0.0f, PK_GI_SPEC_MAX_HISTORY * antilagSpec);
                specVirtual = GI_Mul_NoHistory(specVirtual, 1.0f / wSumVSpec);
                spec.history = min(specVirtual.history, spec.history);
                spec.radiance = min(spec.radiance, specVirtual.radiance);
                spec.ao = min(spec.ao, specVirtual.ao);
            }
        }


        const bool invalidDiff = Test_NaN_EPS6(wSumDiff);
        const bool invalidSpec = Test_NaN_EPS6(wSumSpec);
        packedDiff = invalidDiff ? uint4(0) : GI_Pack_Diff(diff);
        packedSpec = invalidSpec ? uint2(0) : GI_Pack_Spec(spec);
    }

    GI_Store_Packed_Diff(storeCoord, packedDiff);
    GI_Store_Packed_Spec(storeCoord, packedSpec);
}