#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#include includes/SceneGIFiltering.glsl
#include includes/CTASwizzling.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2   coord = GI_ExpandCheckerboardCoord(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    const float  depth = SampleMinZ(coord, 0);
    const float4 normalroughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalroughness.xyz;
    const float  roughness = normalroughness.w;
    const float  depthBias = lerp(0.1f, 0.01f, -normal.z);

    // Without anti firefly filter this isn't needed
    if (!Test_DepthFar(depth))
    {
        return;
    }

    GIDiff diff = GI_Load_Cur_Diff(coord);
    GISpec spec = GI_Load_Cur_Spec(coord);
    GIDiff diffSample = GI_Load_Diff(coord);
    GISpec specSample = GI_Load_Spec(coord);

    const float wDiff = max(1.0f / (diff.history + 1.0f), PK_GI_MIN_ACCUM);
    const float wSpec = max(1.0f / (spec.history + 1.0f), PK_GI_MIN_ACCUM);
    const float maxGainDiff = PK_GI_MAX_LUMA_GAIN / (1.0f - wDiff);
    const float maxGainSpec = PK_GI_MAX_LUMA_GAIN / (1.0f - wSpec);
    const float2 lumaRangeDiff = float2(0.0f, GI_Luminance(diff) + maxGainDiff);
    const float2 lumaRangeSpec = float2(0.0f, GI_Luminance(spec) + maxGainSpec);

    GI_SFLT_ANTI_FIREFLY(coord, normal, depth, depthBias, roughness, lumaRangeDiff, lumaRangeSpec, diffSample, specSample)

    // Interpolate samples
    diff.sh = SH_Interpolate(diff.sh, diffSample.sh, wDiff);
    diff.ao = lerp(diff.ao, diffSample.ao, wDiff);

#if PK_GI_APPROX_ROUGH_SPEC == 1
    if (roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        spec.radiance = lerp(spec.radiance, specSample.radiance, wSpec);
        spec.ao = lerp(spec.ao, specSample.ao, wSpec);
    }

    GI_Store_Diff(coord, diff);
    GI_Store_Spec(coord, spec);
}