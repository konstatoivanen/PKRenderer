#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/Kernels.glsl
#include includes/BRDF.glsl
#include includes/CTASwizzling.glsl

#define SAMPLE_KERNEL PK_POISSON_DISK_32_POW
#define SAMPLE_COUNT 32u

#define FILTER_RADIUS_PASS1	6.0f
#define FILTER_RADIUS_PASS2	3.0f

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf

float2 GetFilterRadiusAndScale(const float depth, const float variance, const float ao, const float history)
{
    // @TODO sort out this non sense...
    const float near_field = saturate(depth * 0.5f);
    float scale = 0.2f + 0.8f * smoothstep(0.0f, 0.3f, pow(ao, 0.4f));
    scale = lerp(0.2f + scale * 0.8f, scale, near_field);
    scale *= 0.05f + 0.95f * variance;
    scale = lerp(scale, 0.5f + scale * 0.5f, 1.0f / history);

    float radius = FILTER_RADIUS_PASS2 * (0.25f + 0.75f * near_field);
    radius *= scale + 1e-5f;
    radius *= sqrt(depth / pk_ProjectionParams.y);

    return float2(radius, scale);
}

float GetSpecularNormalWeight(const float3 c_n, const float3 s_n, const float roughness)
{
    float a0 = PK_HALF_PI * pow2(roughness) / (1.0f + pow2(roughness));
    const float cosa = saturate(dot(c_n, s_n));
    const float a = acos(cosa);
    return float(a < a0);
}


float2 GetSampleUV(const float3 vpos, const float2x3 basis, const float2 rotation, uint index)
{
    const float3 o = SAMPLE_KERNEL[index];
    const float3 v = vpos + basis * rotate2D(o.xy * o.z, rotation);
    const float3 c = mul(pk_MATRIX_P, float4(v, 1.0f)).xyw;
    return (c.xy / c.z) * 0.5f + 0.5f;
}

void ApproximateRoughSpecular(inout GISampleFull filtered, const float3 N, const float3 V, const float R)
{
    float3 worldN = mul(float3x3(pk_MATRIX_I_V), N);
    float3 worldV = mul(float3x3(pk_MATRIX_I_V), V);

    float directionality;
    float3 sh_dir = SH_ToPrimeDir(filtered.diff.sh, directionality);

    float roughness = lerp(1.0f, R, saturate(directionality * 0.666f));
    roughness = sqrt(roughness); // Sample distribution used wrong roughness scale. correct this based on that. :/

    const float3 s_color = SH_ToColor(filtered.diff.sh) * PK_TWO_PI;
    const float3 specular = s_color * BRDF_GGX_SPECULAR(roughness, sh_dir, worldV, worldN);
    const float inter = smoothstep(PK_GI_MIN_ROUGH_SPEC, PK_GI_MAX_ROUGH_SPEC, R);

    filtered.spec.radiance = lerp(filtered.spec.radiance, specular, inter);
}

uint2 GetSwizzledThreadID()
{
    return ThreadGroupTilingX
    (
        gl_NumWorkGroups.xy,
        uint2(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4),
        8u,
        gl_LocalInvocationID.xy,
        gl_WorkGroupID.xy
    );
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(GetSwizzledThreadID());
    const float depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    GISampleFull filtered = GI_Load_SampleFull(coord);
    const float4 c_normalRoughness = SampleViewNormalRoughness(coord);
    const float3 c_normal = c_normalRoughness.xyz;
    const float c_roughness = c_normalRoughness.w;
    const float3 c_vpos = SampleViewPosition(coord, size, depth);
    const float3 c_view = normalize(c_vpos);
    const float2 c_rot = make_rotation(pk_FrameIndex.y * (PK_PI / 3.0f));

    float2 mom;
    mom.x = log(1.0f + SH_ToLuminanceL0(filtered.diff.sh));
    mom.y = pow2(mom.x);

    float w_mom = 1.0f;

    for (int yy = -1; yy <= 1; ++yy)
    for (int xx = -1; xx <= 1; ++xx)
    {
        if (xx == 0 && yy == 0)
        {
            continue;
        }

        const GISampleDiff s_diff = GI_Load_SampleDiff(coord + int2(xx, yy));
        const float s_w = (abs(depth - s_diff.depth) / depth) < 0.02f ? 1.0f : 0.0f;
        const float s_luma = log(1.0f + SH_ToLuminanceL0(s_diff.sh)) * s_w;
        mom += float2(s_luma, pow2(s_luma));
        w_mom += s_w;
    }

    mom /= w_mom;

    const float variance = sqrt(saturate(mom.y - pow2(mom.x)));
    const float2 radiusAndScale = GetFilterRadiusAndScale(depth, variance, filtered.diff.ao, filtered.meta.historyDiff);
    const float radius = radiusAndScale.x;
    const float radius_scale = radiusAndScale.y;
    const bool skip_filter = radius_scale < 0.05f;

    const float normV = 1.0f / (0.05f * depth);
    const float normH = 1.0f / (2.0f * radius * radius);

    const uint step = lerp(uint(max(8.0f - sqrt(radius_scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip_filter);
    uint i = lerp(0u, 0xFFFFu, skip_filter);

    float2x3 basis = ComposeTBFast(c_normal, radius);
    float wSum = 1.0f;

    // Filter Diff
    {
        for (; i < SAMPLE_COUNT; i += step)
        {
            const float2 s_uv = GetSampleUV(c_vpos, basis, c_rot, i);
            const int2 s_px = int2(s_uv * pk_ScreenSize.xy);
            
            GISampleDiff s_diff = GI_Load_SampleDiff(s_px);

            const float3 s_vpos = ClipToViewPos(s_uv * 2 - 1, s_diff.depth);
            const float3 s_ray = c_vpos - s_vpos;

            const float w_h = saturate(1.0f - abs(dot(c_normal, s_ray)) * normV);
            const float w_v = saturate(1.0f - dot(s_ray, s_ray) * normH);
            const float w_s = float(All_InArea(s_px, int2(0), int2(pk_ScreenSize.xy)));

            float w = w_h * w_v * w_s;

            if (!isnan(w) && w > 1e-4f)
            {
                const float3 s_normal = SampleViewNormal(s_px);
                const float w_n = max(0.0f, dot(s_normal, c_normal));
                w *= w_n;

                filtered.diff.sh = SH_Add(filtered.diff.sh, s_diff.sh, w);
                filtered.diff.ao += s_diff.ao * w;
                wSum += w;
            }
        }

        filtered.diff.sh = SH_Scale(filtered.diff.sh, 1.0f / wSum);
        filtered.diff.ao /= wSum;
    }

    // Filter Spec
    #if PK_GI_APPROX_ROUGH_SPEC == 1
    if (c_roughness > PK_GI_MIN_ROUGH_SPEC)
    {
        ApproximateRoughSpecular(filtered, c_normal, c_view, c_roughness);
    }
    
    if (c_roughness < PK_GI_MAX_ROUGH_SPEC)
    #endif
    {
        i = lerp(0u, 0xFFFFu, skip_filter);
        wSum = 1.0f;
        basis = GetPrimeBasisGGX(c_normal, c_view, c_roughness, radius); //@TODO use a different radius?

        for (; i < SAMPLE_COUNT; i += step)
        {
            const float2 s_uv = GetSampleUV(c_vpos, basis, c_rot, i);
            const int2 s_px = int2(s_uv * pk_ScreenSize.xy);

            GISampleSpec s_spec = GI_Load_SampleSpec(s_px);
            const float4 s_nr = SampleViewNormalRoughness(s_px);

            const float3 s_vpos = ClipToViewPos(s_uv * 2 - 1, s_spec.depth);
            const float3 s_ray = c_vpos - s_vpos;

            const float w_h = saturate(1.0f - abs(dot(c_normal, s_ray)) * normV);
            const float w_v = saturate(1.0f - dot(s_ray, s_ray) * normH);
            const float w_r = abs(c_roughness - s_nr.w) / (pow2(c_roughness) * 0.99f + 1e-2f);
            const float w_n = GetSpecularNormalWeight(c_normal, s_nr.xyz, s_nr.w);
            const float w_s = float(All_InArea(s_px, int2(0), int2(pk_ScreenSize.xy)));

            const float w = w_h * w_v * w_s * w_r * w_n;

            if (!isnan(w) && w > 1e-4f)
            {
                filtered.spec.radiance = s_spec.radiance * w;
                filtered.diff.ao += s_spec.ao * w;
                wSum += w;
            }
        }

        filtered.spec.radiance /= wSum;
        filtered.spec.ao /= wSum;
    }


    GI_Store_SampleFull(coord, filtered);
}