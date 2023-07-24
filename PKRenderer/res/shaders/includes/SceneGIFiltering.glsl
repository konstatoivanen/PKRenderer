#pragma once
#include GBuffers.glsl
#include SharedSceneGI.glsl
#include SampleDistribution.glsl
#include Kernels.glsl

// Source https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
float GetGGXDominantFactor(float nv, float linearRoughness)
{
    // @TODO investinage
   return (1.0f - linearRoughness) * (sqrt(1.0f - linearRoughness) + linearRoughness);
   // const float a = 0.298475f * log(39.4115f - 39.0029f * linearRoughness);
   // return saturate(pow( 1.0 - nv, 10.8649f)) * (1.0f - a ) + a;
}

float3 GetGGXDominantDirection(const float3 N, const float3 V, float linearRoughness)
{
    const float factor = GetGGXDominantFactor(abs(dot(N, V)), linearRoughness);
	return normalize(lerp(N, reflect(-V, N), factor));
}

float2x3 GetPrimeBasisGGX(const float3 N, const float3 V, const float R, const float radius, inout float3 P)
{
    P = GetGGXDominantDirection(N, V, sqrt(R));
    const float3 l = reflect(-P, N);
    const float3 t = normalize(cross(N,l));
    const float3 b = cross(l,t);
    return float2x3(t * radius, b * radius);
}

//Source: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf 
float GetGGXLobeHalfAngle(const float R, const float volumeFactor)
{
    return atan(R * volumeFactor / ( 1.0 - volumeFactor));
    //return PK_HALF_PI * R / (1.0f + R);
}

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

// Add small bias (0.01) to prevent sampling from past root texel.
float2 GI_ViewToPrevScreenUV(float3 viewpos) { return ViewToPrevClipUV(viewpos) * int2(pk_ScreenSize.xy) - 0.49f.xx; }

float2 GetRoughnessWeightParams(float roughness)
{
    float2 params;
    params.x = 1.0f / lerp(0.01f, 1.0f, roughness);
    params.y = -roughness * params.x;
    return params;
}

float GetNormalWeightParams(float3 normal, float roughness, float history)
{
    const float halfAngle = GetGGXLobeHalfAngle(roughness, 0.985f);
    return 1.0f / max(halfAngle * lerp(0.5f, 1.0f, 1.0f / (history + 1.0f)), 1e-4f);
}

float2 GetDiskWeightParams(float radius, float depth)
{
    return float2(1.0f / (0.05f * depth),1.0f / (2.0f * pow2(radius)));
}

float GetNormalWeight(float3 normal, float3 sampleNormal, float params)
{
    return exp(-acos(dot(normal, sampleNormal) - 1e-6f) * params);
}

#define MAKE_BILINEAR_WEIGHTS(name, ddxy)                           \
const float name[2][2] =                                            \
{                                                                   \
    { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },   \
    { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },   \
};                                                                  \


#define Test_NaN_EPS4(v) (isnan(v) || v <= 1e-4f)
#define Test_NaN_EPS6(v) (isnan(v) || v <= 1e-6f)

void GI_SFLT_PrevBilinear(float2 screenuv, 
                           int2 coord, 
                           float3 normal, 
                           float depth, 
                           float depthBias, 
                           float roughness,
                           inout float wSumDiff,
                           inout float wSumSpec,
                           inout GIDiff diff,
                           inout GISpec spec)
{
    const float2 roughnessParams = GetRoughnessWeightParams(roughness);
    
    const float2 ddxy = fract(screenuv);
    MAKE_BILINEAR_WEIGHTS(bilinearWeights, ddxy)

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        const int2 xy = coord + int2(xx, yy);

        const float  s_depth = SamplePreviousViewDepth(xy);
        const float4 s_nr = SamplePreviousViewNormalRoughness(xy);
        const GIDiff s_diff = GI_Load_Diff(xy);
        const GISpec s_spec = GI_Load_Spec(xy);

        const float w_b = bilinearWeights[yy][xx];
        const float w_n = dot(normal, s_nr.xyz);
        const float w_r = exp(-abs(s_nr.w * roughnessParams.x + roughnessParams.y));
        const float w_s = float(Test_InScreen(xy) && Test_DepthReproject(depth, s_depth, depthBias));
        const float w_diff = w_s * w_b * w_n;
        const float w_spec = w_s * w_b * w_n * w_r;

        if (!Test_NaN_EPS4(w_diff) && w_n > 0.05f)
        {
            wSumDiff += w_diff;
            diff = GI_Sum(diff, s_diff, w_diff);
        }
        
        if (!Test_NaN_EPS4(w_spec))
        {
            wSumSpec += w_spec;
            spec = GI_Sum(spec, s_spec, w_spec);
        }
    }
}

void GI_SFLT_PrevVirtualSpec(float3 viewpos, 
                              float3 viewdir, 
                              float3 normal, 
                              float depth, 
                              float roughness,
                              float virtualDist,
                              inout float wSumSpec,
                              inout GISpec spec)
{
    const float2 roughnessParams = GetRoughnessWeightParams(roughness);
    const float2 screenuv = GI_ViewToPrevScreenUV(viewpos + viewdir * virtualDist);
    const int2   coord = int2(screenuv);
    const float2 ddxy = fract(screenuv);
    MAKE_BILINEAR_WEIGHTS(bilinearWeights, ddxy)

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        const int2 xy = coord + int2(xx, yy);

        const float  s_depth = SamplePreviousViewDepth(xy);
        const float4 s_nr = SamplePreviousViewNormalRoughness(xy);
        const GISpec s_spec = GI_Load_Spec(xy);

        float w = bilinearWeights[yy][xx];
        w *= 1.0f / (1e-4f + abs(depth - s_depth));
        w *= pow(saturate(dot(normal, s_nr.xyz)), 256.0f);
        w *= exp(-abs(s_nr.w * roughnessParams.x + roughnessParams.y));
        w *= float(Test_InScreen(xy) && Test_DepthFar(s_depth));

        if (!Test_NaN_EPS4(w))
        {
            wSumSpec += w;
            spec = GI_Sum(spec, s_spec, w);
        }
    }
}

void GI_SFLT_PrevBilateralCross(int2 coord, 
                                 float3 normal, 
                                 float depth, 
                                 float depthBias, 
                                 float threshold,
                                 inout float wSumDiff,
                                 inout float wSumSpec,
                                 inout GIDiff diff,
                                 inout GISpec spec)
{
    if (wSumDiff > threshold && wSumSpec > threshold)
    {
        return;
    }

    float filterDiff = float(wSumDiff <= threshold);
    float filterSpec = float(wSumSpec <= threshold);

    for (int yy = -1; yy <= 1; yy++)
    for (int xx = -1; xx <= 1; xx++)
    {
        const int2 xy = coord + int2(xx, yy);

        const float  s_depth = SamplePreviousViewDepth(xy);
        const float3 s_normal = SamplePreviousViewNormal(xy);
        const GIDiff s_diff = GI_Load_Diff(xy);
        const GISpec s_spec = GI_Load_Spec(xy);

        const float w_z = 1.0f / (1e-4f + abs(depth - s_depth));
        const float w_n = dot(normal, s_normal);
        const float w_s = float(Test_InScreen(xy) && Test_DepthReproject(depth, s_depth, depthBias));
        const float w_diff = w_s * w_z * w_n * filterDiff;
        const float w_spec = w_s * w_z * w_n * filterSpec;

        if (!Test_NaN_EPS4(w_diff) && w_n > 0.05f)
        {
            wSumDiff += w_diff;
            diff = GI_Sum(diff, s_diff, w_diff);
        }

        if (!Test_NaN_EPS4(w_spec))
        {
            wSumSpec += w_spec;
            spec = GI_Sum(spec, s_spec, w_spec);
        }
    }
}

void GI_SFLT_AntiFirefly(int2 coord, 
                          float3 normal, 
                          float depth, 
                          float depthBias, 
                          float roughness,
                          float2 lumaRangeDiff,
                          float2 lumaRangeSpec,
                          inout GIDiff diff,
                          inout GISpec spec)
{
    float wSumDiff = 1.0f;
    float wSumSpec = 1.0f;
    float lumaDiff = SH_ToLuminanceL0(diff.sh);
    float lumaSpec = dot(pk_Luminance.rgb, spec.radiance);
    const float2 roughnessParams = GetRoughnessWeightParams(roughness);

    for (int yy = -1; yy <= 1; ++yy)
    for (int xx = -1; xx <= 1; ++xx)
    {
        if (yy == 0 && xx == 0)
        {
            continue;
        }

        // Sample a sparse 5x5 (3x3 with 2px stride) to retain hf noise.
        const int2 xy = coord + int2(xx, yy) * 2;

        const float s_depth = SampleViewDepth(xy);
        const float4 s_nr = SampleViewNormalRoughness(xy);
        const GIDiff s_diff = GI_Load_Diff(xy);
        const GISpec s_spec = GI_Load_Spec(xy);
        
        const float s_lumaDiff = SH_ToLuminanceL0(s_diff.sh);
        const float s_lumaSpec = dot(pk_Luminance.rgb, s_spec.radiance);

        const float w_n = dot(normal, s_nr.xyz);
        const float w_d = 1.0f / (1e-4f + abs(s_depth - depth));
        const float w_r = exp(-abs(s_nr.w * roughnessParams.x + roughnessParams.y));
        const float w_h = float(s_spec.history > 1.0f); // Don't sample ignored ray hits.
        const float w_s = float(Test_InScreen(xy) && Test_DepthReproject(depth, s_depth, depthBias));

        const float w_diff = w_s * w_n * w_d;
        const float w_spec = w_s * pow5(w_n) * w_d * w_r * w_h;

        if (!Test_NaN_EPS4(w_diff) && w_n > 0.05f)
        {
            wSumDiff += w_diff;
            lumaDiff += s_lumaDiff * w_diff;
            diff = GI_Sum(diff, s_diff, w_diff);
        }

        if (!Test_NaN_EPS4(w_spec))
        {
            wSumSpec += w_spec;
            lumaSpec += s_lumaSpec * w_spec;
            spec = GI_Sum(spec, s_spec, w_spec);
        }
    }

    lumaDiff = lumaDiff / wSumDiff + 1e-4f;
    lumaSpec = lumaSpec / wSumSpec + 1e-4f;
    const float scaleDiff = clamp(lumaDiff, lumaRangeDiff.x, lumaRangeDiff.y) / lumaDiff;
    const float scaleSpec = clamp(lumaSpec, lumaRangeSpec.x, lumaRangeSpec.y) / lumaSpec;

    diff = GI_Mul_NoHistory(diff, scaleDiff / wSumDiff);
    spec = GI_Mul_NoHistory(spec, scaleSpec / wSumSpec);
}

void GI_SFLT_HistoryFill(int2 coord, 
                          int mip, 
                          float3 normal, 
                          float depth, 
                          inout float wSumDiff, 
                          inout float wSumSpec, 
                          inout GIDiff diff, 
                          inout GISpec spec)
{
    const int stride0 = 1 << (mip + 1);
    const int stride1 = stride0 / 2;
    const int2 base = (coord - stride1) / stride0;
    const float2 ddxy = float2(coord - stride1 - stride0 * base + 0.5f.xx) / stride0;
    MAKE_BILINEAR_WEIGHTS(bilinearWeights, ddxy)

    for (uint yy = 0; yy <= 1u; ++yy)
    for (uint xx = 0; xx <= 1u; ++xx)
    {
        const uint4 p_diff = GI_Load_Packed_Mip_Diff(base + int2(xx, yy), mip);
        const uint2 p_spec = GI_Load_Packed_Mip_Spec(base + int2(xx, yy), mip);
        const GIDiff s_diff = GI_Unpack_Diff(p_diff);
        const GISpec s_spec = GI_Unpack_Spec(p_spec);
        
        const float s_depth = SampleAvgZ(base + int2(xx, yy), mip + 1);

        float directionality;
        float3 sh_dir = SH_ToPrimeDir(s_diff.sh, directionality);

        // Filter out sh signals that are facing away from surface normal.
        const float w_n = float(dot(normal, sh_dir) > 0.0f || directionality < 0.5f);
        const float w_z = 1.0f / (1e-2f + abs(depth - s_depth));
        const float w_b = bilinearWeights[yy][xx];
        const float w = w_b * w_z;

        if (p_diff.w != 0u && w > 1e-6f)
        {
            wSumDiff += w * w_n;
            wSumSpec += w;
            diff = GI_Sum_NoHistory(diff, s_diff, w * w_n);
            spec = GI_Sum_NoHistory(spec, s_spec, w);
        }
    }
}

float GI_SFLT_EstimateDiffVariance(int2 coord, float depth, GIDiff diff)
{
    float2 mom;
    mom.x = log(1.0f + SH_ToLuminanceL0(diff.sh));
    mom.y = pow2(mom.x);

    float w_mom = 1.0f;

    for (int yy = -1; yy <= 1; ++yy)
    for (int xx = -1; xx <= 1; ++xx)
    {
        if (xx == 0 && yy == 0)
        {
            continue;
        }

        const GIDiff s_diff = GI_Load_Diff(coord + int2(xx, yy));
        const float s_depth = SampleMinZ(coord + int2(xx, yy), 0);

        const float s_w = (abs(depth - s_depth) / depth) < 0.02f ? 1.0f : 0.0f;
        const float s_luma = log(1.0f + SH_ToLuminanceL0(s_diff.sh)) * s_w;
        mom += float2(s_luma, pow2(s_luma));
        w_mom += s_w;
    }

    mom /= w_mom;

    return sqrt(abs(mom.y - pow2(mom.x)));
}

#define GI_SFLT_DISK_DIFF(SFLT_NORMAL, SFLT_DEPTH, SFLT_VIEW, SFLT_VPOS, SFLT_HISTORY, SFLT_STEP, SFLT_SKIP, SFLT_RADIUS, SFLT_OUT) \
{                                                                                                                                \
    float3 disk_normal;                                                                                                          \
    const float2x3 basis = GetPrimeBasisGGX(SFLT_NORMAL, SFLT_VIEW, 1.0f, SFLT_RADIUS, disk_normal);                             \
    const float2 rotation = make_rotation(pk_FrameIndex.y * (PK_PI / 3.0f));                                                     \
    const float halfAngle = GetGGXLobeHalfAngle(1.0f, 0.985f);                                                                   \
                                                                                                                                 \
    const float k_V = 1.0f / (0.05f * SFLT_DEPTH);                                                                               \
    const float k_H = 1.0f / (2.0f * SFLT_RADIUS * SFLT_RADIUS);                                                                 \
    const float k_N = 1.0f / max(halfAngle * lerp(0.5f, 1.0f, 1.0f / (SFLT_HISTORY + 1.0f)), 1e-4f);                             \
                                                                                                                                 \
    float wSum = 1.0f;                                                                                                           \
    uint i = lerp(0u, 0xFFFFu, SFLT_SKIP);                                                                                       \
                                                                                                                                 \
    for (; i < 32u; i += SFLT_STEP)                                                                                              \
    {                                                                                                                            \
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];                                                                         \
        const float2 s_uv = ViewToClipUV(SFLT_VPOS + basis * rotate2D(s_offs.xy, rotation));                                     \
        const int2 s_px = int2(s_uv * int2(pk_ScreenSize.xy));                                                                   \
                                                                                                                                 \
        const float s_depth = SampleMinZ(s_px, 0);                                                                               \
        const float4 s_nr = SampleViewNormalRoughness(s_px);                                                                     \
                                                                                                                                 \
        const float3 s_vpos = UVToViewPos(s_uv, s_depth);                                                                        \
        const float3 s_ray = SFLT_VPOS - s_vpos;                                                                                 \
                                                                                                                                 \
        float w = 1.0f;                                                                                                          \
        w *= saturate(1.0f - abs(dot(disk_normal, s_ray)) * k_V);                                                                \
        w *= saturate(1.0f - dot(s_ray, s_ray) * k_H);                                                                           \
        w *= exp(-acos(dot(SFLT_NORMAL, s_nr.xyz) - 1e-6f) * k_N);                                                               \
        w *= s_offs.z;                                                                                                           \
                                                                                                                                 \
        GIDiff s_data = GI_Load_Diff(s_px);                                                                                      \
                                                                                                                                 \
        if (Test_InScreen(s_uv) && !Test_NaN_EPS4(w))                                                                            \
        {                                                                                                                        \
            wSum += w;                                                                                                           \
            SFLT_OUT = GI_Sum_NoHistory(SFLT_OUT, s_data, w);                                                                    \
        }                                                                                                                        \
    }                                                                                                                            \
                                                                                                                                 \
    SFLT_OUT = GI_Mul_NoHistory(SFLT_OUT, 1.0f / wSum);                                                                          \
}                                                                                                                                \

#define GI_SFLT_DISK_SPEC(SFLT_NORMAL, SFLT_DEPTH, SFLT_ROUGHNESS, SFLT_VIEW, SFLT_VPOS, SFLT_HISTORY, SFLT_STEP, SFLT_SKIP, SFLT_RADIUS, SFLT_OUT) \
{                                                                                                                                                \
    float3 disk_normal;                                                                                                                          \
    const float2x3 basis = GetPrimeBasisGGX(SFLT_NORMAL, SFLT_VIEW, SFLT_ROUGHNESS, SFLT_RADIUS, disk_normal);                                   \
    const float2 rotation = make_rotation(pk_FrameIndex.y * (PK_PI / 3.0f));                                                                     \
    const float halfAngle = GetGGXLobeHalfAngle(SFLT_ROUGHNESS, 0.985f);                                                                         \
                                                                                                                                                 \
    const float k_V = 1.0f / (0.05f * SFLT_DEPTH);                                                                                               \
    const float k_H = 1.0f / (2.0f * SFLT_RADIUS * SFLT_RADIUS);                                                                                 \
    const float k_N = 1.0f / max(halfAngle * lerp(0.5f, 1.0f, 1.0f / (SFLT_HISTORY + 1.0f)), 1e-4f);                                             \
    const float2 k_R = GetRoughnessWeightParams(SFLT_ROUGHNESS);                                                                                 \
                                                                                                                                                 \
    float wSum = 1.0f;                                                                                                                           \
    uint i = lerp(0u, 0xFFFFu, SFLT_SKIP);                                                                                                       \
                                                                                                                                                 \
    for (; i < 32u; i += SFLT_STEP)                                                                                                              \
    {                                                                                                                                            \
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];                                                                                         \
        const float2 s_uv = ViewToClipUV(SFLT_VPOS + basis * rotate2D(s_offs.xy, rotation));                                                     \
        const int2 s_px = int2(s_uv * int2(pk_ScreenSize.xy));                                                                                   \
                                                                                                                                                 \
        const float s_depth = SampleMinZ(s_px, 0);                                                                                               \
        const float4 s_nr = SampleViewNormalRoughness(s_px);                                                                                     \
                                                                                                                                                 \
        const float3 s_vpos = UVToViewPos(s_uv, s_depth);                                                                                        \
        const float3 s_ray = SFLT_VPOS - s_vpos;                                                                                                 \
                                                                                                                                                 \
        float w = 1.0f;                                                                                                                          \
        w *= saturate(1.0f - abs(dot(disk_normal, s_ray)) * k_V);                                                                                \
        w *= saturate(1.0f - dot(s_ray, s_ray) * k_H);                                                                                           \
        w *= exp(-acos(dot(SFLT_NORMAL, s_nr.xyz) - 1e-6f) * k_N);                                                                               \
        w *= exp(-abs(s_nr.w * k_R.x + k_R.y));                                                                                                  \
        w *= s_offs.z;                                                                                                                           \
                                                                                                                                                 \
        GISpec s_data = GI_Load_Spec(s_px);                                                                                                      \
                                                                                                                                                 \
        if (Test_InScreen(s_uv) && !Test_NaN_EPS4(w))                                                                                            \
        {                                                                                                                                        \
            wSum += w;                                                                                                                           \
            SFLT_OUT = GI_Sum_NoHistory(SFLT_OUT, s_data, w);                                                                                    \
        }                                                                                                                                        \
    }                                                                                                                                            \
                                                                                                                                                 \
    SFLT_OUT = GI_Mul_NoHistory(SFLT_OUT, 1.0f / wSum);                                                                                          \
}                                                                                                                                                \