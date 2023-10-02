#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#define PK_GI_LOAD_LVL 0
#define PK_GI_STORE_LVL 1

#include includes/SceneGIFiltering.glsl
#include includes/BRDF.glsl
#include includes/CTASwizzling.glsl

void ApproximateRoughSpecular(const float3 vN, const float3 vV, const float roughness, const GIDiff diff, inout GISpec spec)
{
    float3 wN = mul(float3x3(pk_MATRIX_I_V), vN);
    float3 wV = mul(float3x3(pk_MATRIX_I_V), vV);

    float directionality;
    float3 primedir = SH_ToPrimeDir(diff.sh, directionality);

    const float newRoughness = sqrt(lerp(1.0f, pow2(roughness), saturate(directionality * 0.666f)));
    const float3 color = SH_ToColor(diff.sh) * PK_TWO_PI;
    const float3 specular = color * EvaluateBxDF_Specular(wN, wV, newRoughness, primedir);
    const float inter = smoothstep(PK_GI_MIN_ROUGH_SPEC, PK_GI_MAX_ROUGH_SPEC, roughness);

    spec.ao = lerp(spec.ao, diff.ao, inter);
    spec.history = lerp(spec.history, diff.history, inter);
    spec.radiance = lerp(spec.radiance, specular, inter);
}

void HistoryFill(const int2 coord,
    const float depth,
    const float3 normal,
    const float roughness,
    inout GIDiff outDiff,
    inout GISpec outSpec)
{
    const float historyDiff = outDiff.history;

#if PK_GI_APPROX_ROUGH_SPEC == 1
    // Rough spec might have invalid data.
    const float historySpec = lerp(PK_GI_SPEC_MAX_HISTORY, outSpec.history, roughness < PK_GI_MAX_ROUGH_SPEC);
#else
    const float historySpec = outSpec.history;
#endif

    const float interpolant = min(historyDiff, historySpec) / PK_GI_HISTORY_FILL_THRESHOLD;
    const float level = min(3.0f - 1e-4f, 4.0f - 4.0f * interpolant);

    [[branch]]
    if (level > 0.0f)
    {
        GIDiff diff = GIDiff(pk_ZeroSH, 0.0f, historyDiff);
        GISpec spec = GISpec(0.0f.xxx, 0.0f, historySpec);
        float wSum = 0.0f;

        GI_SF_HISTORY_MIP(coord, level, normal, depth, wSum, diff, spec)

        [[branch]]
        if (int(historyDiff) <= PK_GI_HISTORY_FILL_THRESHOLD && !Test_NaN_EPS6(wSum))
        {
            outDiff.sh = GI_Mul_NoHistory(diff, 1.0f / wSum).sh;
        }

        [[branch]]
        if (int(historySpec) <= PK_GI_HISTORY_FILL_THRESHOLD && !Test_NaN_EPS6(wSum))
        {
            spec = GI_Mul_NoHistory(spec, 1.0f / wSum);
            // Dont use history fill for smooth surfaces.
            outSpec.radiance = lerp(outSpec.radiance, spec.radiance, smoothstep(0.1f, 0.6f, roughness));
        }
    }
}

void CheckerboardFillDiff(const int2 baseCoord, const GIDiff history, const GIDiff filtered)
{
    const int2 ncoord = GI_ExpandCheckerboardCoord(uint2(baseCoord), 1u);
    const int2 hcoord = int2(baseCoord.x + pk_ScreenSize.x / 2, baseCoord.y);

    const GIDiff neighbour = GI_Load_Diff(hcoord);
    const float alpha = GI_Alpha(neighbour);

    const GIDiff n_history = GI_Interpolate(neighbour, history, pow2(alpha));
    const GIDiff n_filtered = GI_Interpolate(neighbour, filtered, alpha);

    GI_Store_Diff(ncoord, n_history);
    GI_Store_Resolved_Diff(ncoord, SampleWorldNormal(ncoord), n_filtered);
}

void CheckerboardFillSpec(const int2 baseCoord, const GISpec history, const GISpec filtered)
{
    const int2 ncoord = GI_ExpandCheckerboardCoord(uint2(baseCoord), 1u);
    const int2 hcoord = int2(baseCoord.x + pk_ScreenSize.x / 2, baseCoord.y);

    const GISpec neighbour = GI_Load_Spec(hcoord);
    const float alpha = GI_Alpha(neighbour);

    const GISpec n_history = GI_Interpolate(neighbour, history, pow2(alpha));
    const GISpec n_filtered = GI_Interpolate(neighbour, filtered, alpha);

    GI_Store_Spec(ncoord, n_history);
    GI_Store_Resolved_Spec(ncoord, n_filtered);
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const float depth = SampleMinZ(coord, 0);

    const float4 normalRoughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalRoughness.xyz;
    const float3 wnormal = ViewToWorldDir(normal);
    const float roughness = normalRoughness.w;
    const float3 viewpos = SampleViewPosition(coord, depth);
    const float3 viewdir = normalize(viewpos);

    GIDiff f_diff = GI_Load_Diff(baseCoord);
    GISpec f_spec = GI_Load_Spec(baseCoord);
    GIDiff h_diff = f_diff;
    GISpec h_spec = f_spec;

    HistoryFill(coord, depth, wnormal, normalRoughness.w, f_diff, f_spec);

    // Filter Diff
    {

        //float variance = 0.0f;
        //GI_SF_DIFF_VARIANCE(coord, depth, f_diff, variance)
        //
        //const float2 radiusAndScale = GI_GetDiskFilterRadiusAndScale(depth, variance, f_diff.ao, f_diff.history);
        //const float scale = radiusAndScale.y;
        //const float radius = radiusAndScale.x * (scale + 1e-4f);
        //const bool skip = f_diff.history > 30.0f || scale < 0.05f;
        //const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
        //GI_SF_DISK_DIFF(normal, depth, viewdir, viewpos, f_diff.history, step, skip, radius, f_diff)

        const float alpha = GI_Alpha(h_diff) * 0.25f;
        h_diff = GI_ClampLuma(h_diff, GI_MaxLuma(f_diff, alpha));
        h_diff.sh = SH_Interpolate(h_diff.sh, f_diff.sh, alpha);
        f_diff.ao = lerp(h_diff.ao, 0.5f + f_diff.ao * 0.5f, alpha);

        h_diff.history += 1.0f;
        f_diff.history += 1.0f;

        GI_Store_Diff(coord, h_diff);
        GI_Store_Resolved_Diff(coord, wnormal, f_diff);
#if defined(PK_GI_CHECKERBOARD_TRACE)
        CheckerboardFillDiff(baseCoord, h_diff, f_diff);
#endif
    }

    // Filter Spec
    {
#if PK_GI_APPROX_ROUGH_SPEC == 1
        if (roughness > PK_GI_MIN_ROUGH_SPEC)
        {
            ApproximateRoughSpecular(normal, viewdir, roughness, f_diff, f_spec);
            h_spec = f_spec;
        }

        if (roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
        {
            /*
            // @TODO Calculate different radius for this as diffuse variance is hardly usable & roughness is more of a relevant factor.
            const float2 radiusAndScale = GI_GetDiskFilterRadiusAndScale(depth, 0.0f, spec.ao, spec.history);
            const float scale = radiusAndScale.y * sqrt(roughness);
            const float radius = radiusAndScale.x * (scale + 1e-4f);
            const bool skip = scale < 0.05f;
            const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
            GI_SF_DISK_SPEC(normal, depth, roughness, viewdir, viewpos, spec.history, step, skip, radius, spec)
            */

            const float alpha = GI_Alpha(h_spec) * 0.25f;
            h_spec = GI_ClampLuma(h_spec, GI_MaxLuma(f_spec, alpha));
            h_spec.radiance = lerp(h_spec.radiance, f_spec.radiance, alpha);
            h_spec.history += 1.0f;
            f_spec.ao = lerp(h_spec.ao, 0.5f + f_spec.ao * 0.5f, alpha);
        }

        GI_Store_Spec(coord, h_spec);
        GI_Store_Resolved_Spec(coord, f_spec);
#if defined(PK_GI_CHECKERBOARD_TRACE)
        CheckerboardFillSpec(baseCoord, h_spec, f_spec);
#endif
    }

}