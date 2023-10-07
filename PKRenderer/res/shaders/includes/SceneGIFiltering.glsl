#pragma once
#include GBuffers.glsl
#include SharedSceneGI.glsl
#include Kernels.glsl

// Source https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
float GI_GetSpecularDominantFactor(float nv, float roughness)
{
    // @TODO investinage
   float linearRoughness = sqrt(roughness);
   return (1.0f - linearRoughness) * (sqrt(1.0f - linearRoughness) + linearRoughness);
   // const float a = 0.298475f * log(39.4115f - 39.0029f * linearRoughness);
   // return saturate(pow( 1.0 - nv, 10.8649f)) * (1.0f - a ) + a;
}

float3 GI_GetSpecularDominantDirection(const float3 N, const float3 V, float roughness)
{
    const float factor = GI_GetSpecularDominantFactor(abs(dot(N, V)), roughness);
    return normalize(lerp(N, reflect(-V, N), factor));
}

float2x3 GI_GetSpecularDominantBasis(const float3 N, const float3 V, const float R, const float radius, inout float3 P)
{
    P = GI_GetSpecularDominantDirection(N, V, R);
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
    float a = pow(acos01sq, PK_GI_SPEC_ANTILAG_CURVE);
    float b = 1.1 + pow2(roughness);
    float parallaxSensitivity = (b + a) / (b - a);
    float powerScale = 1.0 + parallax * parallaxSensitivity;
    float f = 1.0 - exp2(-200.0 * pow2(roughness));
    f *= pow(roughness, PK_GI_SPEC_ANTILAG_BASE_POWER * powerScale);
    return lerp(PK_GI_SPEC_ANTILAG_MIN, PK_GI_SPEC_ANTILAG_MAX, f);
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
    scale = lerp(scale, 0.75f + scale * 0.25f, 1.0f / (history + 1.0f));

    float radius = PK_GI_DISK_FILTER_RADIUS * (0.25f + 0.75f * near_field);
    radius *= sqrt(depth / pk_ProjectionParams.y);
    radius *= sqrt(3840.0 * pk_ScreenParams.z);	

    return float2(radius, scale);
}

void GI_GetMipSampler(const int2 coord, int level, inout int2 outBaseCoord, inout float2 outDdxy)
{
    const int width = 1 << level;
    const int hwidth = width >> 1;
    outBaseCoord = ((coord + hwidth) >> level) - 1;
    outDdxy = (((coord + hwidth) & (width - 1)) + 0.5f.xx) / width;
}

float4 GI_GetBilinearWeights(float2 f) { return float4((1.0 - f.x) * (1.0 - f.y), f.x * (1.0 - f.y), (1.0 - f.x) * f.y, f.x * f.y); }

/* Profiling revealed that deferring these to functions yielded significantly worse performance, so they're macros for now. */

#define GI_SF_DIFF_VARIANCE(SF_COORD, SF_DEPTH, SF_DIFF, SF_OUT)                                \
{                                                                                               \
    float2 mom;                                                                                 \
    mom.x = GI_LogLuminance(SF_DIFF);                                                           \
    mom.y = pow2(mom.x);                                                                        \
    float w_mom = 1.0f;                                                                         \
                                                                                                \
    for (int yy = -1; yy <= 1; ++yy)                                                            \
    for (int xx = -1; xx <= 1; ++xx)                                                            \
    {                                                                                           \
        if (xx != 0 || yy != 0)                                                                 \
        {                                                                                       \
            const int2 xy = SF_COORD + int2(xx, yy);                                            \
            const float s_depth = SampleMinZ(xy, 0);                                            \
            const float s_w = lerp(0.0f, 1.0f, (abs(SF_DEPTH - s_depth) / SF_DEPTH) < 0.02f);   \
            mom += make_moments(GI_LogLuminance(GI_Load_Diff(xy)) * s_w);                       \
            w_mom += s_w;                                                                       \
        }                                                                                       \
    }                                                                                           \
                                                                                                \
    mom /= w_mom;                                                                               \
    SF_OUT = sqrt(abs(mom.y - pow2(mom.x)));                                                    \
}                                                                                               \

#define GI_SF_DISK_DIFF(SF_NORMAL, SF_DEPTH, SF_VIEW, SF_VPOS, SF_HISTORY, SF_STEP, SF_SKIP, SF_RADIUS, SF_OUT) \
{                                                                                                               \
    const float2x3 basis = make_TB(SF_NORMAL, SF_RADIUS);                                                       \
    const float2 rotation = GI_GetRandomRotation();                                                             \
    const float k_N = GI_GetNormalWeightParams(SF_NORMAL, 1.0f, SF_HISTORY);                                    \
    const float2 k_D = GI_GetDiskWeightParams(SF_RADIUS, SF_DEPTH);                                             \
                                                                                                                \
    float wSum = 1.0f;                                                                                          \
    uint i = lerp(0u, 0xFFFFu, SF_SKIP);                                                                        \
                                                                                                                \
    for (; i < 32u; i += SF_STEP)                                                                               \
    {                                                                                                           \
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];                                                        \
        float2 s_uv = ViewToClipUV(SF_VPOS + basis * rotate2D(s_offs.xy, rotation));                            \
        s_uv = 1.0f - abs(1.0f - abs(s_uv));                                                                    \
        const int2   s_gpx = GI_CollapseCheckerboardCoord(s_uv * pk_ScreenSize.xy, 0);                          \
        const int2   s_px = GI_ExpandCheckerboardCoord(s_gpx);                                                  \
        const float4 s_nr = SampleViewNormalRoughness(s_px);                                                    \
        const float3 s_ray = SF_VPOS - UVToViewPos(s_uv, SampleMinZ(s_px, 0));                                  \
                                                                                                                \
        float w = 1.0f;                                                                                         \
        w *= saturate(1.0f - abs(dot(SF_NORMAL, s_ray)) * k_D.x);                                               \
        w *= saturate(1.0f - dot(s_ray, s_ray) * k_D.y);                                                        \
        w *= exp(-acos(dot(SF_NORMAL, s_nr.xyz) - 1e-6f) * k_N);                                                \
        w *= exp( -0.66 * s_offs.z *  s_offs.z);                                                                \
        w = lerp(0.0f, w, !Test_NaN_EPS4(w));                                                                   \
                                                                                                                \
        SF_OUT = GI_Sum_NoHistory(SF_OUT, GI_Load_Diff(s_gpx), w);                                              \
        wSum += w;                                                                                              \
    }                                                                                                           \
                                                                                                                \
    SF_OUT = GI_Mul_NoHistory(SF_OUT, 1.0f / wSum);                                                             \
}                                                                                                               \

#define GI_SF_DISK_SPEC(SF_NORMAL, SF_DEPTH, SF_ROUGHNESS, SF_VIEW, SF_VPOS, SF_HISTORY, SF_STEP, SF_SKIP, SF_RADIUS, SF_OUT)   \
{                                                                                                                               \
    float3 disk_normal;                                                                                                         \
    const float2x3 basis = GI_GetSpecularDominantBasis(SF_NORMAL, SF_VIEW, SF_ROUGHNESS, SF_RADIUS, disk_normal);               \
    const float2 rotation = GI_GetRandomRotation();                                                                             \
    const float k_N = GI_GetNormalWeightParams(SF_NORMAL, SF_ROUGHNESS, SF_HISTORY);                                            \
    const float2 k_D = GI_GetDiskWeightParams(SF_RADIUS, SF_DEPTH);                                                             \
    const float2 k_R = GI_GetRoughnessWeightParams(SF_ROUGHNESS);                                                               \
                                                                                                                                \
    float wSum = 1.0f;                                                                                                          \
    uint i = lerp(0u, 0xFFFFu, SF_SKIP);                                                                                        \
                                                                                                                                \
    for (; i < 32u; i += SF_STEP)                                                                                               \
    {                                                                                                                           \
        const float3 s_offs = PK_POISSON_DISK_32_POW[i];                                                                        \
        const float2 s_uv = ViewToClipUV(SF_VPOS + basis * rotate2D(s_offs.xy, rotation));                                      \
        const int2   s_px = int2(s_uv * pk_ScreenSize.xy);                                                                      \
        const float4 s_nr = SampleViewNormalRoughness(s_px);                                                                    \
        const float3 s_ray = SF_VPOS - UVToViewPos(s_uv, SampleMinZ(s_px, 0));                                                  \
                                                                                                                                \
        float w = 1.0f;                                                                                                         \
        w *= saturate(1.0f - abs(dot(disk_normal, s_ray)) * k_D.x);                                                             \
        w *= saturate(1.0f - dot(s_ray, s_ray) * k_D.y);                                                                        \
        w *= exp(-acos(dot(SF_NORMAL, s_nr.xyz) - 1e-6f) * k_N);                                                                \
        w *= exp(-abs(s_nr.w * k_R.x + k_R.y));                                                                                 \
        w *= exp( -0.66 * s_offs.z *  s_offs.z);                                                                                \
        #if PK_GI_APPROX_ROUGH_SPEC == 1                                                                                        \
        w *= float(s_nr.w < PK_GI_MAX_ROUGH_SPEC);                                                                              \
        #endif                                                                                                                  \
        w = lerp(0.0f, w, Test_InScreen(s_uv) && !Test_NaN_EPS4(w));                                                            \
                                                                                                                                \
        SF_OUT = GI_Sum_NoHistory(SF_OUT, GI_Load_Spec(s_px), w);                                                               \
        wSum += w;                                                                                                              \
    }                                                                                                                           \
                                                                                                                                \
    SF_OUT = GI_Mul_NoHistory(SF_OUT, 1.0f / wSum);                                                                             \
}                                                                                                                               \