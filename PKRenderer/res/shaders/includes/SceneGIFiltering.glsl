#pragma once
#include GBuffers.glsl
#include SharedSceneGI.glsl
#include SampleDistribution.glsl
#include Kernels.glsl

// Source https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
float GI_GetSpecularDominantFactor(float nv, float linearRoughness)
{
    // @TODO investinage
   return (1.0f - linearRoughness) * (sqrt(1.0f - linearRoughness) + linearRoughness);
   // const float a = 0.298475f * log(39.4115f - 39.0029f * linearRoughness);
   // return saturate(pow( 1.0 - nv, 10.8649f)) * (1.0f - a ) + a;
}

float3 GI_GetSpecularDominantDirection(const float3 N, const float3 V, float linearRoughness)
{
    const float factor = GI_GetSpecularDominantFactor(abs(dot(N, V)), linearRoughness);
	return normalize(lerp(N, reflect(-V, N), factor));
}

float2x3 GI_GetSpecularDominantBasis(const float3 N, const float3 V, const float R, const float radius, inout float3 P)
{
    P = GI_GetSpecularDominantDirection(N, V, sqrt(R));
    const float3 l = reflect(-P, N);
    const float3 t = normalize(cross(N,l));
    const float3 b = cross(l,t);
    return float2x3(t * radius, b * radius);
}

//Source: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf 
//return PK_HALF_PI * R / (1.0f + R);
float GI_GetSpecularLobeHalfAngle(const float R, const float volumeFactor) { return atan(R * volumeFactor / ( 1.0 - volumeFactor)); }

float GI_GetParallax(float3 viewdir_cur, float3 viewdir_pre)
{
    float cosa = saturate(dot(viewdir_cur, viewdir_pre));
    float parallax = sqrt(1.0 - cosa * cosa) / max(cosa, 1e-6);
    return parallax / pk_DeltaTime.x;
}

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
float GI_GetAntilagSpecular(float roughness, float nv, float parallax)
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

// Add small bias (0.01) to prevent sampling from past root texel.
float2 GI_ViewToPrevScreenUV(const float3 viewpos) { return ViewToPrevClipUV(viewpos) * int2(pk_ScreenSize.xy) - 0.49f.xx; }

float2 GI_GetRandomRotation() { return make_rotation((uintBitsToFloat(pk_FrameRandom.x & 0x007fffffu | 0x3f800000u) - 1.0f) * PK_TWO_PI); }

float2 GI_GetDiskWeightParams(float radius, float depth) { return float2(1.0f / (0.05f * depth), 1.0f / (2.0f * pow2(radius))); }

float2 GI_GetRoughnessWeightParams(const float roughness)
{
    float2 params;
    params.x = 1.0f / lerp(0.01f, 1.0f, roughness);
    params.y = -roughness * params.x;
    return params;
}

float GI_GetNormalWeightParams(const float3 normal, const float roughness, const float history)
{
    const float halfAngle = GI_GetSpecularLobeHalfAngle(roughness, 0.985f);
    return 1.0f / max(halfAngle * lerp(0.5f, 1.0f, 1.0f / (history + 1.0f)), 1e-4f);
}

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s9985-exploring-ray-traced-future-in-metro-exodus.pdf
float2 GI_GetDiskFilterRadiusAndScale(const float depth, const float variance, const float ao, const float history)
{
    // @TODO sort out this nonsense...
    const float near_field = saturate(depth * 0.5f);
    float scale = 0.2f + 0.8f * smoothstep(0.0f, 0.3f, pow(ao, 0.125f));
    scale = lerp(0.2f + scale * 0.8f, scale, near_field);
    scale *= 0.05f + 0.95f * variance;
    scale = lerp(scale, 0.5f + scale * 0.5f, 1.0f / history);

    float radius = PK_GI_DISK_FILTER_RADIUS * (0.25f + 0.75f * near_field);
    radius *= sqrt(depth / pk_ProjectionParams.y);
    radius *= sqrt(3840.0 * pk_ScreenParams.z);	

    return float2(radius, scale);
}

#define MAKE_BILINEAR_WEIGHTS(name, ddxy)                           \
const float name[2][2] =                                            \
{                                                                   \
    { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },   \
    { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },   \
};                                                                  \


#define Test_NaN_EPS4(v) (isnan(v) || v <= 1e-4f)
#define Test_NaN_EPS6(v) (isnan(v) || v <= 1e-6f)
#define Test_EPS4(v) (v <= 1e-4f)
#define Test_EPS6(v) (v <= 1e-6f)

// Profiling revealed that deferring these to functions yielded significantly worse performance, so they're macros for now.

#define GI_SFLT_REPRO_BILINEAR(SFLT_UV, SFLT_COORD, SFLT_NORMAL, SFLT_DEPTH, SFLT_DBIAS, SFLT_ROUGHNESS, SFLT_WSUM_DIFF, SFLT_WSUM_SPEC, SFLT_OUT_DIFF, SFLT_OUT_SPEC)  \
{                                                                                                                                                                       \
    const float2 roughnessParams = GI_GetRoughnessWeightParams(SFLT_ROUGHNESS);                                                                                         \
    const float2 ddxy = fract(SFLT_UV);                                                                                                                                 \
    MAKE_BILINEAR_WEIGHTS(bilinearWeights, ddxy)                                                                                                                        \
                                                                                                                                                                        \
    for (int yy = 0; yy <= 1; ++yy)                                                                                                                                     \
    for (int xx = 0; xx <= 1; ++xx)                                                                                                                                     \
    {                                                                                                                                                                   \
        const int2 xy = SFLT_COORD + int2(xx, yy);                                                                                                                      \
        const float  s_depth = SamplePreviousViewDepth(xy);                                                                                                             \
        const float4 s_nr = SamplePreviousViewNormalRoughness(xy);                                                                                                      \
                                                                                                                                                                        \
        const float w_b = bilinearWeights[yy][xx];                                                                                                                      \
        const float w_n = pow5(dot(SFLT_NORMAL, s_nr.xyz));                                                                                                             \
        const float w_r = exp(-abs(s_nr.w * roughnessParams.x + roughnessParams.y));                                                                                    \
        const float w_s = float(Test_InScreen(xy) && Test_DepthReproject(SFLT_DEPTH, s_depth, SFLT_DBIAS));                                                             \
        float w_diff = w_s * w_b * w_n;                                                                                                                                 \
        float w_spec = w_s * w_b * w_n * w_r;                                                                                                                           \
        w_diff = lerp(0.0f, w_diff, !Test_NaN_EPS6(w_diff) && w_n > 0.05f);                                                                                             \
        w_spec = lerp(0.0f, w_spec, !Test_NaN_EPS6(w_spec));                                                                                                            \
                                                                                                                                                                        \
        SFLT_OUT_DIFF = GI_Sum(SFLT_OUT_DIFF, GI_Load_Diff(xy), w_diff);                                                                                                \
        SFLT_OUT_SPEC = GI_Sum(SFLT_OUT_SPEC, GI_Load_Spec(xy), w_spec);                                                                                                \
        SFLT_WSUM_DIFF += w_diff;                                                                                                                                       \
        SFLT_WSUM_SPEC += w_spec;                                                                                                                                       \
    }                                                                                                                                                                   \
}                                                                                                                                                                       \

#define GI_SFLT_REPRO_VIRTUAL_SPEC(SFLT_VPOS, SFLT_VIEW, SFLT_NORMAL, SFLT_DEPTH, SFLT_ROUGHNESS, SFLT_VIRT_DIST, SFLT_OUT_WSUM, SFLT_OUT)  \
{                                                                                                                                           \
    const float2 roughnessParams = GI_GetRoughnessWeightParams(SFLT_ROUGHNESS);                                                             \
    const float2 screenuv = GI_ViewToPrevScreenUV(SFLT_VPOS + SFLT_VIEW * SFLT_VIRT_DIST);                                                  \
    const int2   coordSpec = int2(screenuv);                                                                                                \
    const float2 ddxy = fract(screenuv);                                                                                                    \
    MAKE_BILINEAR_WEIGHTS(bilinearWeights, ddxy)                                                                                            \
                                                                                                                                            \
    for (int yy = 0; yy <= 1; ++yy)                                                                                                         \
    for (int xx = 0; xx <= 1; ++xx)                                                                                                         \
    {                                                                                                                                       \
        const int2 xy = coordSpec + int2(xx, yy);                                                                                           \
        const float  s_depth = SamplePreviousViewDepth(xy);                                                                                 \
        const float4 s_nr = SamplePreviousViewNormalRoughness(xy);                                                                          \
                                                                                                                                            \
        float w = bilinearWeights[yy][xx];                                                                                                  \
        w *= 1.0f / (1e-4f + abs(SFLT_DEPTH - s_depth));                                                                                    \
        w *= pow(saturate(dot(SFLT_NORMAL, s_nr.xyz)), 256.0f);                                                                             \
        w *= exp(-abs(s_nr.w * roughnessParams.x + roughnessParams.y));                                                                     \
        w = lerp(0.0f, w, Test_InScreen(xy) && Test_DepthFar(s_depth) && !Test_NaN_EPS6(w));                                                \
                                                                                                                                            \
        SFLT_OUT = GI_Sum(SFLT_OUT, GI_Load_Spec(xy), w);                                                                                   \
        SFLT_OUT_WSUM += w;                                                                                                                 \
    }                                                                                                                                       \
}                                                                                                                                           \

// A more relaxed filter for when bilinear reprojection filter fails
#define GI_SFLT_REPRO_BILATERAL_CROSS(SFLT_COORD, SFLT_NORMAL, SFLT_DEPTH, SFLT_DBIAS, SFLT_WSUM_DIFF, SFLT_WSUM_SPEC, SFLT_OUT_DIFF, SFLT_OUT_SPEC)    \
{                                                                                                                                                       \
    bool filterDiff = Test_EPS6(SFLT_WSUM_DIFF);                                                                                                        \
    bool filterSpec = Test_EPS6(SFLT_WSUM_SPEC);                                                                                                        \
                                                                                                                                                        \
    if (filterDiff || filterSpec)                                                                                                                       \
    {                                                                                                                                                   \
        for (int yy = -1; yy <= 1; yy++)                                                                                                                \
        for (int xx = -1; xx <= 1; xx++)                                                                                                                \
        {                                                                                                                                               \
            const int2 xy = SFLT_COORD + int2(xx, yy);                                                                                                  \
            const float  s_depth = SamplePreviousViewDepth(xy);                                                                                         \
            const float3 s_normal = SamplePreviousViewNormal(xy);                                                                                       \
                                                                                                                                                        \
            const float w_z = 1.0f / (1e-4f + abs(SFLT_DEPTH - s_depth));                                                                               \
            const float w_n = dot(SFLT_NORMAL, s_normal);                                                                                               \
            const float w_s = float(Test_InScreen(xy) && Test_DepthReproject(SFLT_DEPTH, s_depth, SFLT_DBIAS));                                         \
            float w_diff = w_s * w_z * w_n;                                                                                                             \
            float w_spec = w_s * w_z * w_n;                                                                                                             \
            w_diff = lerp(0.0f, w_diff, filterDiff && !Test_NaN_EPS4(w_diff) && w_n > 0.05f);                                                           \
            w_spec = lerp(0.0f, w_spec, filterSpec && !Test_NaN_EPS4(w_spec));                                                                          \
                                                                                                                                                        \
            SFLT_OUT_DIFF = GI_Sum(SFLT_OUT_DIFF, GI_Load_Diff(xy), w_diff);                                                                            \
            SFLT_OUT_SPEC = GI_Sum(SFLT_OUT_SPEC, GI_Load_Spec(xy), w_spec);                                                                            \
            SFLT_WSUM_DIFF += w_diff;                                                                                                                   \
            SFLT_WSUM_SPEC += w_spec;                                                                                                                   \
        }                                                                                                                                               \
    }                                                                                                                                                   \
}                                                                                                                                                       \

#define GI_SFLT_ANTI_FIREFLY(SFLT_COORD, SFLT_NORMAL, SFLT_DEPTH, SFLT_DBIAS, SFLT_ROUGHNESS, SFLT_LUMA_RANGE_DIFF, SFLT_LUMA_RANGE_SPEC, SFLT_OUT_DIFF, SFLT_OUT_SPEC) \
{                                                                                                                                                                       \
    float wSumDiff = 1.0f;                                                                                                                                              \
    float wSumSpec = 1.0f;                                                                                                                                              \
    float lumaDiff = GI_Luminance(SFLT_OUT_DIFF);                                                                                                                       \
    float lumaSpec = GI_Luminance(SFLT_OUT_SPEC);                                                                                                                       \
    const float2 roughnessParams = GI_GetRoughnessWeightParams(SFLT_ROUGHNESS);                                                                                         \
                                                                                                                                                                        \
    [[unroll]]                                                                                                                                                          \
    for (int i = 0; i < 9; ++i)                                                                                                                                         \
    {                                                                                                                                                                   \
        if (i != 4)                                                                                                                                                     \
        {                                                                                                                                                               \
            /* Sample a sparse 5x5 (3x3 with 1px padding on axis aligned samples) to retain hf noise & to only sample shaded hits if using checkerboard rendering. */   \
            const int2 xy = SFLT_COORD + int2(i % 3, i / 3) * (1 + (i % 2));                                                                                            \
            const float s_depth = SampleMinZ(xy, 0);                                                                                                                    \
            const float4 s_nr = SampleViewNormalRoughness(xy);                                                                                                          \
            const GIDiff s_diff = GI_Load_Diff(xy);                                                                                                                     \
            const GISpec s_spec = GI_Load_Spec(xy);                                                                                                                     \
                                                                                                                                                                        \
            /* use higher power here to avoid sampling around corners*/                                                                                                 \
            const float w_n = pow5(dot(SFLT_NORMAL, s_nr.xyz));                                                                                                         \
            const float w_d = 1.0f / (1e-4f + abs(s_depth - SFLT_DEPTH));                                                                                               \
            const float w_r = exp(-abs(s_nr.w * roughnessParams.x + roughnessParams.y));                                                                                \
            const float w_h = float(s_spec.history > 1.0f); /* Don't sample ignored ray hits. */                                                                        \
            const float w_s = float(Test_InScreen(xy) && Test_DepthReproject(SFLT_DEPTH, s_depth, SFLT_DBIAS));                                                         \
                                                                                                                                                                        \
            float w_diff = w_s * w_n * w_d;                                                                                                                             \
            float w_spec = w_s * w_n * w_d * w_r * w_h;                                                                                                                 \
            w_diff = lerp(0.0f, w_diff, !Test_NaN_EPS6(w_diff) && w_n > 0.05f);                                                                                         \
            w_spec = lerp(0.0f, w_spec, !Test_NaN_EPS6(w_spec));                                                                                                        \
                                                                                                                                                                        \
            SFLT_OUT_DIFF = GI_Sum(SFLT_OUT_DIFF, s_diff, w_diff);                                                                                                      \
            SFLT_OUT_SPEC = GI_Sum(SFLT_OUT_SPEC, s_spec, w_spec);                                                                                                      \
            lumaDiff += GI_Luminance(s_diff) * w_diff;                                                                                                                  \
            lumaSpec += GI_Luminance(s_spec) * w_spec;                                                                                                                  \
            wSumDiff += w_diff;                                                                                                                                         \
            wSumSpec += w_spec;                                                                                                                                         \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
                                                                                                                                                                        \
    lumaDiff = lumaDiff / wSumDiff + 1e-4f;                                                                                                                             \
    lumaSpec = lumaSpec / wSumSpec + 1e-4f;                                                                                                                             \
    SFLT_OUT_DIFF = GI_Mul_NoHistory(SFLT_OUT_DIFF, (clamp(lumaDiff, SFLT_LUMA_RANGE_DIFF.x, SFLT_LUMA_RANGE_DIFF.y) / lumaDiff) / wSumDiff);                           \
    SFLT_OUT_SPEC = GI_Mul_NoHistory(SFLT_OUT_SPEC, (clamp(lumaSpec, SFLT_LUMA_RANGE_SPEC.x, SFLT_LUMA_RANGE_SPEC.y) / lumaSpec) / wSumSpec);                           \
}                                                                                                                                                                       \

#define GI_SFLT_HISTORY_FILL(SFLT_COORD, SFLT_MIP, SFLT_NORMAL, SFLT_DEPTH, SFLT_OUT_WSUM_DIFF, SFLT_OUT_WSUM_SPEC, SFLT_OUT_DIFF, SFLT_OUT_SPEC)   \
{                                                                                                                                                   \
    const int stride0 = 1 << (SFLT_MIP + 1);                                                                                                        \
    const int stride1 = stride0 / 2;                                                                                                                \
    const int2 base = (SFLT_COORD - stride1) / stride0;                                                                                             \
    const float2 ddxy = float2(SFLT_COORD - stride1 - stride0 * base + 0.5f.xx) / stride0;                                                          \
    MAKE_BILINEAR_WEIGHTS(bilinearWeights, ddxy)                                                                                                    \
                                                                                                                                                    \
    for (uint yy = 0; yy <= 1u; ++yy)                                                                                                               \
    for (uint xx = 0; xx <= 1u; ++xx)                                                                                                               \
    {                                                                                                                                               \
        const uint4 p_diff = GI_Load_Packed_Mip_Diff(base + int2(xx, yy), SFLT_MIP);                                                                \
        const uint2 p_spec = GI_Load_Packed_Mip_Spec(base + int2(xx, yy), SFLT_MIP);                                                                \
        const GIDiff s_diff = GI_Unpack_Diff(p_diff);                                                                                               \
        const GISpec s_spec = GI_Unpack_Spec(p_spec);                                                                                               \
        const float s_depth = SampleAvgZ(base + int2(xx, yy), SFLT_MIP + 1);                                                                        \
                                                                                                                                                    \
        float directionality;                                                                                                                       \
        float3 sh_dir = SH_ToPrimeDir(s_diff.sh, directionality);                                                                                   \
                                                                                                                                                    \
        /* Filter out sh signals that are facing away from surface normal.*/                                                                        \
        const float w_n = float(dot(SFLT_NORMAL, sh_dir) > 0.0f || directionality < 0.5f);                                                          \
        const float w_z = 1.0f / (1e-2f + abs(SFLT_DEPTH - s_depth));                                                                               \
        const float w_b = bilinearWeights[yy][xx];                                                                                                  \
        const float w = w_b * w_z;                                                                                                                  \
                                                                                                                                                    \
        if (p_diff.w != 0u && w > 1e-6f)                                                                                                            \
        {                                                                                                                                           \
            SFLT_OUT_DIFF = GI_Sum_NoHistory(SFLT_OUT_DIFF, s_diff, w * w_n);                                                                       \
            SFLT_OUT_SPEC = GI_Sum_NoHistory(SFLT_OUT_SPEC, s_spec, w);                                                                             \
            SFLT_OUT_WSUM_DIFF += w * w_n;                                                                                                          \
            SFLT_OUT_WSUM_SPEC += w;                                                                                                                \
        }                                                                                                                                           \
    }                                                                                                                                               \
}                                                                                                                                                   \


#define GI_SFLT_DIFF_VARIANCE(SFLT_COORD, SFLT_DEPTH, SFLT_DIFF, SFLT_OUT)                              \
{                                                                                                       \
    float2 mom;                                                                                         \
    mom.x = GI_LogLuminance(SFLT_DIFF);                                                                 \
    mom.y = pow2(mom.x);                                                                                \
    float w_mom = 1.0f;                                                                                 \
                                                                                                        \
    for (int yy = -1; yy <= 1; ++yy)                                                                    \
    for (int xx = -1; xx <= 1; ++xx)                                                                    \
    {                                                                                                   \
        if (xx != 0 || yy != 0)                                                                         \
        {                                                                                               \
            const int2 xy = SFLT_COORD + int2(xx, yy);                                                  \
            const float s_depth = SampleMinZ(xy, 0);                                                    \
            const float s_w = lerp(0.0f, 1.0f, (abs(SFLT_DEPTH - s_depth) / SFLT_DEPTH) < 0.02f);       \
            const float s_luma = GI_LogLuminance(GI_Load_Diff(xy)) * s_w;                               \
            mom += float2(s_luma, pow2(s_luma));                                                        \
            w_mom += s_w;                                                                               \
        }                                                                                               \
    }                                                                                                   \
                                                                                                        \
    mom /= w_mom;                                                                                       \
    SFLT_OUT = sqrt(abs(mom.y - pow2(mom.x)));                                                          \
}                                                                                                       \

#define GI_SFLT_DISK_DIFF(SFLT_NORMAL, SFLT_DEPTH, SFLT_VIEW, SFLT_VPOS, SFLT_HISTORY, SFLT_STEP, SFLT_SKIP, SFLT_RADIUS, SFLT_OUT) \
{                                                                                                                                   \
    const float2x3 basis = ComposeTBFast(SFLT_NORMAL, SFLT_RADIUS);                                                                 \
    const float2 rotation = GI_GetRandomRotation();                                                                                 \
    const float k_N = GI_GetNormalWeightParams(SFLT_NORMAL, 1.0f, SFLT_HISTORY);                                                    \
    const float2 k_D = GI_GetDiskWeightParams(SFLT_RADIUS, SFLT_DEPTH);                                                             \
                                                                                                                                    \
    float wSum = 1.0f;                                                                                                              \
    uint i = lerp(0u, 0xFFFFu, SFLT_SKIP);                                                                                          \
                                                                                                                                    \
    for (; i < 32u; i += SFLT_STEP)                                                                                                 \
    {                                                                                                                               \
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];                                                                            \
        const float2 s_uv = ViewToClipUV(SFLT_VPOS + basis * rotate2D(s_offs.xy, rotation));                                        \
        const int2   s_px = int2(s_uv * pk_ScreenSize.xy);                                                                          \
        const float4 s_nr = SampleViewNormalRoughness(s_px);                                                                        \
        const float3 s_ray = SFLT_VPOS - UVToViewPos(s_uv, SampleMinZ(s_px, 0));                                                    \
                                                                                                                                    \
        float w = 1.0f;                                                                                                             \
        w *= saturate(1.0f - abs(dot(SFLT_NORMAL, s_ray)) * k_D.x);                                                                 \
        w *= saturate(1.0f - dot(s_ray, s_ray) * k_D.y);                                                                            \
        w *= exp(-acos(dot(SFLT_NORMAL, s_nr.xyz) - 1e-6f) * k_N);                                                                  \
        w *= exp( -0.66 * s_offs.z *  s_offs.z);                                                                                    \
        w = lerp(0.0f, w, Test_InScreen(s_uv) && !Test_NaN_EPS4(w));                                                                \
                                                                                                                                    \
        SFLT_OUT = GI_Sum_NoHistory(SFLT_OUT, GI_Load_Diff(s_px), w);                                                               \
        wSum += w;                                                                                                                  \
    }                                                                                                                               \
                                                                                                                                    \
    SFLT_OUT = GI_Mul_NoHistory(SFLT_OUT, 1.0f / wSum);                                                                             \
}                                                                                                                                   \

#define GI_SFLT_DISK_SPEC(SFLT_NORMAL, SFLT_DEPTH, SFLT_ROUGHNESS, SFLT_VIEW, SFLT_VPOS, SFLT_HISTORY, SFLT_STEP, SFLT_SKIP, SFLT_RADIUS, SFLT_OUT) \
{                                                                                                                                                   \
    float3 disk_normal;                                                                                                                             \
    const float2x3 basis = GI_GetSpecularDominantBasis(SFLT_NORMAL, SFLT_VIEW, SFLT_ROUGHNESS, SFLT_RADIUS, disk_normal);                           \
    const float2 rotation = GI_GetRandomRotation();                                                                                                 \
    const float k_N = GI_GetNormalWeightParams(SFLT_NORMAL, SFLT_ROUGHNESS, SFLT_HISTORY);                                                          \
    const float2 k_D = GI_GetDiskWeightParams(SFLT_RADIUS, SFLT_DEPTH);                                                                             \
    const float2 k_R = GI_GetRoughnessWeightParams(SFLT_ROUGHNESS);                                                                                 \
                                                                                                                                                    \
    float wSum = 1.0f;                                                                                                                              \
    uint i = lerp(0u, 0xFFFFu, SFLT_SKIP);                                                                                                          \
                                                                                                                                                    \
    for (; i < 32u; i += SFLT_STEP)                                                                                                                 \
    {                                                                                                                                               \
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];                                                                                            \
        const float2 s_uv = ViewToClipUV(SFLT_VPOS + basis * rotate2D(s_offs.xy, rotation));                                                        \
        const int2   s_px = int2(s_uv * pk_ScreenSize.xy);                                                                                          \
        const float4 s_nr = SampleViewNormalRoughness(s_px);                                                                                        \
        const float3 s_ray = SFLT_VPOS - UVToViewPos(s_uv, SampleMinZ(s_px, 0));                                                                    \
                                                                                                                                                    \
        float w = 1.0f;                                                                                                                             \
        w *= saturate(1.0f - abs(dot(disk_normal, s_ray)) * k_D.x);                                                                                 \
        w *= saturate(1.0f - dot(s_ray, s_ray) * k_D.y);                                                                                            \
        w *= exp(-acos(dot(SFLT_NORMAL, s_nr.xyz) - 1e-6f) * k_N);                                                                                  \
        w *= exp(-abs(s_nr.w * k_R.x + k_R.y));                                                                                                     \
        w *= exp( -0.66 * s_offs.z *  s_offs.z);                                                                                                    \
        w = lerp(0.0f, w, Test_InScreen(s_uv) && !Test_NaN_EPS4(w));                                                                                \
                                                                                                                                                    \
        SFLT_OUT = GI_Sum_NoHistory(SFLT_OUT, GI_Load_Spec(s_px), w);                                                                               \
        wSum += w;                                                                                                                                  \
    }                                                                                                                                               \
                                                                                                                                                    \
    SFLT_OUT = GI_Mul_NoHistory(SFLT_OUT, 1.0f / wSum);                                                                                             \
}                                                                                                                                                   \
