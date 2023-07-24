#version 460
#pragma PROGRAM_COMPUTE
#include includes/SceneGIFiltering.glsl
#include includes/BRDF.glsl
#include includes/CTASwizzling.glsl

#define FILTER_RADIUS_PASS1	6.0f
#define FILTER_RADIUS_PASS2	3.0f

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s9985-exploring-ray-traced-future-in-metro-exodus.pdf
// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf

float2 GetFilterRadiusAndScale(const float depth, const float variance, const float ao, const float history)
{
    // @TODO sort out this non sense...
    const float near_field = saturate(depth * 0.5f);
    float scale = 0.2f + 0.8f * smoothstep(0.0f, 0.3f, pow(ao, 0.125f));
    scale = lerp(0.2f + scale * 0.8f, scale, near_field);
    scale *= 0.05f + 0.95f * variance;
    scale = lerp(scale, 0.5f + scale * 0.5f, 1.0f / history);

    float radius = FILTER_RADIUS_PASS2 * (0.25f + 0.75f * near_field);
    radius *= scale + 1e-5f;
    radius *= sqrt(depth / pk_ProjectionParams.y);

    return float2(radius, scale);
}

void ApproximateRoughSpecular(const SH sh, const float3 N, const float3 V, const float R, inout GISpec spec)
{
    float3 wN = mul(float3x3(pk_MATRIX_I_V), N);
    float3 wV = mul(float3x3(pk_MATRIX_I_V), V);

    float directionality;
    float3 sh_dir = SH_ToPrimeDir(sh, directionality);

    const float roughness = sqrt(lerp(1.0f, R * R, saturate(directionality * 0.666f)));

    const float3 s_color = SH_ToColor(sh) * PK_TWO_PI;
    const float3 specular = s_color * BRDF_GGX_SPECULAR_APPROX(wN, wV, roughness, sh_dir);
    const float inter = smoothstep(PK_GI_MIN_ROUGH_SPEC, PK_GI_MAX_ROUGH_SPEC, R);

    spec.radiance = lerp(spec.radiance, specular, inter);
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
    const float depth = SampleViewDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    GIDiff c_diff = GI_Load_Diff(coord);
    GISpec c_spec = GI_Load_Spec(coord);

    const float4 c_normalRoughness = SampleViewNormalRoughness(coord);
    const float3 c_normal = c_normalRoughness.xyz;
    const float c_roughness = c_normalRoughness.w;
    const float3 c_vpos = SampleViewPosition(coord, size, depth);
    const float3 c_view = normalize(c_vpos);

    const float variance = GI_SFLT_EstimateDiffVariance(coord, depth, c_diff);
    const float2 radiusAndScale = GetFilterRadiusAndScale(depth, variance, c_diff.ao, c_diff.history);
    const float radius = radiusAndScale.x;
    const float radius_scale = radiusAndScale.y;
    const bool skip_filter = radius_scale < 0.05f;
    const uint step = lerp(uint(max(8.0f - sqrt(radius_scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip_filter);

    // Filter diff
    GI_SFLT_DISK_DIFF(c_normal, depth, c_view, c_vpos, c_diff.history, step, skip_filter, radius, c_diff)

    // Filter Spec
    // @TODO Approx causes weird blobs that stick around. possibly due to prefilter?
    // @TODO Calculate different radius for this as diffuse variance is hardly usable & roughness is more of a relevant factor.
#if PK_GI_APPROX_ROUGH_SPEC == 1
    if (c_roughness > PK_GI_MIN_ROUGH_SPEC)
    {
        ApproximateRoughSpecular(c_diff.sh, c_normal, c_view, c_roughness, c_spec);
    }

    if (c_roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        GI_SFLT_DISK_SPEC(c_normal, depth, c_roughness, c_view, c_vpos, c_spec.history, step, true, radius, c_spec)
    }

    GI_Store_Diff(coord, c_diff);
    GI_Store_Spec(coord, c_spec);
}