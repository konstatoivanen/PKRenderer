#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/SampleDistribution.glsl

float GetParallax(float3 viewdir_cur, float3 viewdir_pre)
{
    float cosa = saturate(dot(viewdir_cur, viewdir_pre));
    float parallax = sqrt(1.0 - cosa * cosa) / max(cosa, 1e-6);
    return parallax / pk_DeltaTime.x;
}

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
float GetAntilagSpecular(float roughness, float nv, float parallax)
{
    float acos01sq = saturate(1.0f - nv);
    float a = pow(acos01sq, PK_GI_SPEC_ACCUM_CURVE);
    float b = 1.1 + pow2(roughness);
    float parallaxSensitivity = (b + a) / (b - a);
    float powerScale = 1.0 + parallax * parallaxSensitivity;
    float f = 1.0 - exp2(-200.0 * roughness * roughness);
    f *= pow(roughness, PK_GI_SPEC_ACCUM_BASE_POWER * powerScale);
    return lerp(PK_GI_SPEC_ACCUM_MIN, PK_GI_SPEC_ACCUM_MAX, f);
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2  size = int2(pk_ScreenSize.xy);
    const int2  c_coord = int2(gl_GlobalInvocationID.xy);
    const float c_depth = SampleViewDepth(c_coord);

    // Far clip or new backbuffer
    if (pk_FrameIndex.y == 0u || !Test_DepthFar(c_depth))
    {
        GI_Store_Packed_Diff(c_coord, uint4(0));
        GI_Store_Packed_Spec(c_coord, uint2(0));
        return;
    }

    const float4 c_normalroughness = SampleViewNormalRoughness(c_coord);
    const float3 c_normal = c_normalroughness.xyz;
    const float  c_roughness = c_normalroughness.w;
    const float  c_zbias = lerp(0.1f, 0.01f, -c_normal.z);
    const float3 c_vpos = SampleViewPosition(c_coord, size, c_depth);
    const float  c_vdst = length(c_vpos);
    const float3 c_vdir = c_vpos / c_vdst;
    const float  c_nv = dot(c_normal, -c_vdir);

    const float parallax = GetParallax(c_vdir, normalize(c_vpos - pk_ViewSpaceCameraDelta.xyz));
    const float antilag_spec = GetAntilagSpecular(c_roughness, c_nv, parallax);
    const float antilag_diff = 1.0f;

    const float k_R0 = 1.0f / lerp(0.01f, 1.0f, c_roughness);
    const float k_R1 = -c_roughness * k_R0;

    // Bias to prevent drifting effect.
    const float2 c_fcoord_prev = ViewToPrevClipUV(c_vpos) * size - 0.49f.xx;
    const int2   c_coord_prev = int2(c_fcoord_prev);

    GIDiff c_diff = pk_Zero_GIDiff;
    GISpec c_spec = pk_Zero_GISpec;
    float curvature = 0.0f;
    float diff_wsum = 0.0f;
    float spec_wsum = 0.0f;

    // Reconstruct diff & naive spec
    {
        const float2 ddxy = fract(c_fcoord_prev);
        const float bilinearWeights[2][2] =
        {
            { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
            { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
        };

        for (int yy = 0; yy <= 1; ++yy)
        for (int xx = 0; xx <= 1; ++xx)
        {
            const int2   xy = c_coord_prev + int2(xx, yy);
            const float  s_depth = SamplePreviousViewDepth(xy);
            const float3 s_normal = SamplePreviousViewNormal(xy);
            const GIDiff s_diff = GI_Load_Diff(xy);
            const GISpec s_spec = GI_Load_Spec(xy);

            const float  w_n = dot(c_normal, s_normal);
            const float  w = bilinearWeights[yy][xx] * w_n;

            if (Test_InScreen(xy) && Test_DepthReproject(c_depth, s_depth, c_zbias) && !isnan(w) && w > 1e-4f)
            {
                curvature += pow(w_n, 256.0f);
                diff_wsum += w;
                c_spec = GI_Sum(c_spec, s_spec, w);
                c_diff = GI_Sum(c_diff, s_diff, w);
            }
        }

        spec_wsum = diff_wsum;
    }
    
    // Reproject spec (if enough data from naive reprojection)
    // @TODO better curvature filtering
    // When this works it looks nice... but when it doesn't the ghosting is really distracting.
#if PK_GI_SPEC_VIRT_REPROJECT == 1
    if (diff_wsum > 1e-4f)
    {

        float spec_dst = (c_spec.ao / diff_wsum) * PK_GI_RAY_MAX_DISTANCE;
        spec_dst *= pow(curvature / 4.0f, 1024.0f);
        spec_dst *= GetGGXDominantFactor(c_nv, sqrt(c_roughness));

        const float2 spec_fcoord = ViewToPrevClipUV(c_vpos + c_vdir * spec_dst) * size - 0.49f.xx;
        const int2   spec_coord = int2(spec_fcoord);
        
        const float2 ddxy = fract(spec_fcoord);
        const float bilinearWeights[2][2] =
        {
            { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
            { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
        };

        spec_wsum = 0.0f;
        GISpec r_spec = pk_Zero_GISpec;

        for (int yy = 0; yy <= 1; ++yy)
        for (int xx = 0; xx <= 1; ++xx)
        {
            const int2   xy = spec_coord + int2(xx, yy);
            const float  s_depth = SamplePreviousViewDepth(xy);
            const float4 s_nr = SamplePreviousViewNormalRoughness(xy);
            const GISpec s_spec = GI_Load_Spec(xy);

            float w = bilinearWeights[yy][xx];
            w *= 1.0f / (1e-4f + abs(c_depth - s_depth));
            w *= pow(saturate(dot(c_normal, s_nr.xyz)), 256.0f);
            w *= exp(-abs(s_nr.w * k_R0 + k_R1));

            if (Test_InScreen(xy) && Test_DepthFar(s_depth) && !isnan(w) && w > 1e-4f)
            {
                spec_wsum += w;
                r_spec = GI_Sum(r_spec, s_spec, w);
            }
        }

        // @TODO get the min of previous repro & this to eliminate ghosting.
        const float inter = spec_wsum / max(spec_wsum, diff_wsum);
        c_spec.radiance = lerp(c_spec.radiance, r_spec.radiance, inter);
        c_spec.history = lerp(c_spec.history, r_spec.history, inter);
        c_spec.ao = lerp(c_spec.ao, r_spec.ao, inter);
        spec_wsum = lerp(diff_wsum, spec_wsum, inter);
    }
#endif

    // Try to find valid samples with a bilateral cross filter
    if (diff_wsum <= 1e-4f)
    {
        diff_wsum = 0.0f;

        for (int yy = -1; yy <= 1; yy++)
        for (int xx = -1; xx <= 1; xx++)
        {
            const int2   xy = c_coord_prev + int2(xx, yy);
            const float  s_depth = SamplePreviousViewDepth(xy);
            const float3 s_normal = SamplePreviousViewNormal(xy);
            const GIDiff s_diff = GI_Load_Diff(xy);
            const GISpec s_spec = GI_Load_Spec(xy);
            
            const float w = (1.0f / (1e-4f + abs(c_depth - s_depth))) * dot(c_normal, s_normal);

            if (Test_InScreen(xy) && Test_DepthReproject(c_depth, s_depth, c_zbias) && !isnan(w) && w > 1e-4f)
            {
                diff_wsum += w;
                c_spec = GI_Sum(c_spec, s_spec, w);
                c_diff = GI_Sum(c_diff, s_diff, w);
            }
        }

        spec_wsum = diff_wsum;
    }

    // Normalize weights
    if (diff_wsum > 1e-4f)
    {
        c_spec.history = clamp((c_spec.history / spec_wsum) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilag_spec);
        c_diff.history = clamp((c_diff.history / diff_wsum) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilag_diff);
        c_spec = GI_Mul_NoHistory(c_spec, 1.0f / spec_wsum);
        c_diff = GI_Mul_NoHistory(c_diff, 1.0f / diff_wsum);
    }

    const bool diff_invalid = Any_IsNaN(c_diff.sh.Y) || Any_IsNaN(c_diff.sh.CoCg) || isnan(c_diff.ao) || isnan(c_diff.history);
    const bool spec_invalid = Any_IsNaN(c_spec.radiance) || isnan(c_spec.ao) || isnan(c_spec.history);
    GI_Store_Packed_Diff(c_coord, diff_invalid ? uint4(0) : GI_Pack_Diff(c_diff));
    GI_Store_Packed_Spec(c_coord, spec_invalid ? uint2(0) : GI_Pack_Spec(c_spec));
}