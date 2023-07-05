#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/Kernels.glsl
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

void ApproximateRoughSpecular(const SH sh, const float3 N, const float3 V, const float R, inout GISampleSpec spec)
{
    float3 worldN = mul(float3x3(pk_MATRIX_I_V), N);
    float3 worldV = mul(float3x3(pk_MATRIX_I_V), V);

    float directionality;
    float3 sh_dir = SH_ToPrimeDir(sh, directionality);

    float roughness = lerp(1.0f, R * R, saturate(directionality * 0.666f));
    roughness = sqrt(roughness);

    const float3 s_color = SH_ToColor(sh) * PK_TWO_PI;
    const float3 specular = s_color * BRDF_GGX_SPECULAR_APPROX(roughness, sh_dir, worldV, worldN);
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
    const float depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    GISampleDiff c_diff = GI_Load_SampleDiff(coord);
    GISampleSpec c_spec = GI_Load_SampleSpec(coord);

    const float4 c_normalRoughness = SampleViewNormalRoughness(coord);
    const float3 c_normal = c_normalRoughness.xyz;
    const float c_roughness = c_normalRoughness.w;
    const float3 c_vpos = SampleViewPosition(coord, size, depth);
    const float3 c_view = normalize(c_vpos);

    float2 mom;
    mom.x = log(1.0f + SH_ToLuminanceL0(c_diff.sh));
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
        const float s_depth = SampleMinZ(coord + int2(xx, yy), 0);

        const float s_w = (abs(depth - s_depth) / depth) < 0.02f ? 1.0f : 0.0f;
        const float s_luma = log(1.0f + SH_ToLuminanceL0(s_diff.sh)) * s_w;
        mom += float2(s_luma, pow2(s_luma));
        w_mom += s_w;
    }

    mom /= w_mom;

    const float variance = sqrt(abs(mom.y - pow2(mom.x)));
    const float2 radiusAndScale = GetFilterRadiusAndScale(depth, variance, c_diff.ao, c_diff.history);
    const float radius = radiusAndScale.x;
    const float radius_scale = radiusAndScale.y;
    const bool skip_filter = radius_scale < 0.05f;
    const uint step = lerp(uint(max(8.0f - sqrt(radius_scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip_filter);

    // Filter diff
    {
        #define SFLT_WEIGH_ROUGHNESS 0
        #define SFLT_NORMAL c_normal
        #define SFLT_DEPTH depth 
        #define SFLT_ROUGHNESS 1.0f
        #define SFLT_VIEW c_view 
        #define SFLT_VPOS c_vpos
        #define SFLT_HISTORY c_diff.history
        #define SFLT_STEP step
        #define SFLT_SKIP skip_filter
        #define SFLT_RADIUS radius
        #define SFLT_DATA_TYPE GISampleDiff
        #define SFLT_DATA_LOAD(coord) GI_Load_SampleDiff(coord)

        #define SFLT_DATA_SUM(data, w)\
            c_diff.sh = SH_Add(c_diff.sh, data.sh, w);\
            c_diff.ao += data.ao * w;\

        #define SFLT_DATA_DIV(wSum)\
            c_diff.sh = SH_Scale(c_diff.sh, 1.0f / wSum);\
            c_diff.ao /= wSum;\

        #include includes/SpatialFilter.glsl
    }

    // Filter Spec
#if PK_GI_APPROX_ROUGH_SPEC == 1
    if (c_roughness > PK_GI_MIN_ROUGH_SPEC)
    {
        ApproximateRoughSpecular(c_diff.sh, c_normal, c_view, c_roughness, c_spec);
    }

    if (c_roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        #define SFLT_WEIGH_ROUGHNESS 1
        #define SFLT_NORMAL c_normal
        #define SFLT_DEPTH depth 
        #define SFLT_ROUGHNESS c_roughness
        #define SFLT_VIEW c_view 
        #define SFLT_VPOS c_vpos
        #define SFLT_HISTORY c_spec.history
        #define SFLT_STEP step
        #define SFLT_SKIP skip_filter
        #define SFLT_RADIUS radius
        #define SFLT_DATA_TYPE GISampleSpec
        #define SFLT_DATA_LOAD(coord) GI_Load_SampleSpec(coord)
        
        #define SFLT_DATA_SUM(data, w)\
            c_spec.radiance += data.radiance * w;\
            c_spec.ao += data.ao * w;\
        
        #define SFLT_DATA_DIV(wSum)\
            c_spec.radiance /= wSum;\
            c_spec.ao /= wSum;\

        #include includes/SpatialFilter.glsl
    }

    GI_Store_SampleDiff(coord, c_diff);
    GI_Store_SampleSpec(coord, c_spec);
}