#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/Kernels.glsl
#include includes/BRDF.glsl

#define SAMPLE_KERNEL PK_POISSON_DISK_32_POW
#define SAMPLE_COUNT 32u

#define FILTER_RADIUS_PASS1	6.0f
#define FILTER_RADIUS_PASS2	0.25f

void ApproximateRoughSpecular(inout GISampleFull filtered, const float3 N, const float3 V, const float R)
{
#if PK_GI_APPROX_ROUGH_SPEC == 1
    float3 worldN = mul(float3x3(pk_MATRIX_I_V), N);
    float3 worldV = mul(float3x3(pk_MATRIX_I_V), V);

    float sh_len = length(filtered.diff.sh.Y.wyz);
    float3 sh_dir = filtered.diff.sh.Y.wyz / sh_len;

    float directionality = sh_len / (filtered.diff.sh.Y.x + 1e-6f);
    float roughness = lerp(1.0f, R, saturate(directionality * 0.666f));
    roughness = sqrt(roughness); // Sample distribution used wrong roughness scale. correct this based on that. :/

    const float3 s_color = SHToColor(filtered.diff.sh) * PK_TWO_PI;
    const float3 specular = s_color * BRDF_GGX_SPECULAR(roughness, sh_dir, worldV, worldN);
    const float inter = smoothstep(PK_GI_MIN_ROUGH_SPEC, PK_GI_MAX_ROUGH_SPEC, R);

    filtered.spec.radiance = lerp(filtered.spec.radiance, specular, smoothstep(0.4f, 0.5f, R));
#endif
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
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
    const float3x3 c_tbn = ComposeTBNFast(c_normal);

    const float2 rotation = make_rotation(pk_FrameIndex.y * (PK_PI / 3.0f));

    float f = saturate((c_view.z / pk_ProjectionParams.y) / 2.0);
    float radius_scale = lerp(0.2, 1.0, saturate(filtered.diff.ao / 0.3));
    radius_scale = lerp(0.2 + radius_scale * 0.8, radius_scale, f);
    radius_scale = lerp(0.5 + radius_scale * 0.5, radius_scale, 1.0f / (filtered.meta.historyDiff + 1.0f));

    float radius = FILTER_RADIUS_PASS2 * lerp(0.25, 1.0, f);
    radius *= radius_scale;
    radius *= sqrt(depth / pk_ProjectionParams.y);

    const float normV = 1.0f / (0.05f * depth);
    const float normH = 1.0f / (2.0f * radius * radius);

    const uint step = radius_scale < 0.05f ? 999u : uint(max(8.0f - sqrt(radius_scale) * 7.0f, 1.0f) + 0.01f);
    const uint first = radius_scale < 0.05f ? 999u : 0u;

    float wSumDiff = 1.0f;
    float wSumSpec = 1.0f;

    for (uint i = first; i < SAMPLE_COUNT; i += step)
    {
        const float3 s_offs = SAMPLE_KERNEL[i];
        const float3 s_pos = c_vpos + c_tbn * float3(rotate2D(s_offs.xy * s_offs.z * radius, rotation), 0.0f);

        float3 s_clip = mul(pk_MATRIX_P, float4(s_pos, 1.0f)).xyw;
        s_clip.xy /= s_clip.z;

        const float2 s_uv = s_clip.xy * 0.5f + 0.5f;
        const int2 s_px = int2(s_uv * pk_ScreenSize.xy);
        const float3 s_vpos = SampleViewPosition(s_px, size);
        const float3 s_ray = c_vpos - s_vpos;

        const float w_h = saturate(1.0f - abs(dot(c_normal, s_ray)) * normV);
        const float w_v = saturate(1.0f - dot(s_ray, s_ray) * normH);
        const float w_s = float(All_Equal(saturate(s_uv), s_uv));

        float w = w_h * w_v * w_s;

        if (!isnan(w) && w > 1e-4f)
        {
            const float3 s_normal = SampleViewNormal(s_px);
            const float w_n = max(0.0f, dot(s_normal, c_normal));
            w *= w_n;

            GISampleDiff s_diff = GI_Load_SampleDiff(int2(s_uv * pk_ScreenSize.xy));

            filtered.diff.sh = SHAdd(filtered.diff.sh, s_diff.sh, w);
            filtered.diff.ao += s_diff.ao * w;
            wSumDiff += w;
        }
    }

    filtered.diff.sh = SHScale(filtered.diff.sh, 1.0f / wSumDiff);
    filtered.diff.ao /= wSumDiff;

    ApproximateRoughSpecular(filtered, c_normal, c_view, c_roughness);

    GI_Store_SampleFull(coord, filtered);
}