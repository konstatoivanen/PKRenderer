#version 460
#pragma PROGRAM_COMPUTE
#include includes/SceneGIFiltering.glsl
#include includes/BRDF.glsl
#include includes/CTASwizzling.glsl

void ApproximateRoughSpecular(const float3 N, const float3 V, const float R, const GIDiff diff, inout GISpec spec)
{
    float3 wN = mul(float3x3(pk_MATRIX_I_V), N);
    float3 wV = mul(float3x3(pk_MATRIX_I_V), V);

    float directionality;
    float3 primedir = SH_ToPrimeDir(diff.sh, directionality);

    const float roughness = sqrt(lerp(1.0f, R * R, saturate(directionality * 0.666f)));

    const float3 color = SH_ToColor(diff.sh) * PK_TWO_PI;
    const float3 specular = color * BRDF_GGX_SPECULAR_APPROX(wN, wV, roughness, primedir);
    const float inter = smoothstep(PK_GI_MIN_ROUGH_SPEC, PK_GI_MAX_ROUGH_SPEC, R);

    spec.ao = lerp(spec.ao, diff.ao, inter);
    spec.history = lerp(spec.history, diff.history, inter);
    spec.radiance = lerp(spec.radiance, specular, inter);
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_4, 8u));
    const float depth = SampleMinZ(coord, 0);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    GIDiff diff = GI_Load_Diff(coord);
    GISpec spec = GI_Load_Spec(coord);

    const float4 normalRoughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalRoughness.xyz;
    const float roughness = normalRoughness.w;
    const float3 viewpos = SampleViewPosition(coord, size, depth);
    const float3 viewdir = normalize(viewpos);

    // Filter Diff
    {
        float variance = 0.0f;
        GI_SFLT_DIFF_VARIANCE(coord, depth, diff, variance)

        const float2 radiusAndScale = GI_GetDiskFilterRadiusAndScale(depth, variance, diff.ao, diff.history);
        const float scale = radiusAndScale.y;
        const float radius = radiusAndScale.x * (scale + 1e-4f);
        const bool skip = scale < 0.05f;
        const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
        GI_SFLT_DISK_DIFF(normal, depth, viewdir, viewpos, diff.history, step, true, radius, diff)
    }

    // Filter Spec
#if PK_GI_APPROX_ROUGH_SPEC == 1
    if (roughness > PK_GI_MIN_ROUGH_SPEC)
    {
        ApproximateRoughSpecular(normal, viewdir, roughness, diff, spec);
    }

    if (roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        // @TODO Calculate different radius for this as diffuse variance is hardly usable & roughness is more of a relevant factor.
        const float2 radiusAndScale = GI_GetDiskFilterRadiusAndScale(depth, 0.0f, spec.ao, spec.history);
        const float scale = radiusAndScale.y * sqrt(roughness);
        const float radius = radiusAndScale.x * (scale + 1e-4f);
        const bool skip = scale < 0.05f;
        const uint step = lerp(uint(max(8.0f - sqrt(scale) * 7.0f, 1.0f) + 0.01f), 0xFFFFu, skip);
        GI_SFLT_DISK_SPEC(normal, depth, roughness, viewdir, viewpos, spec.history, step, true, radius, spec)
    }

    GI_Store_Diff(coord, diff);
    GI_Store_Spec(coord, spec);
}