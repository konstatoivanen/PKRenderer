#version 460
#pragma PROGRAM_COMPUTE
#include includes/SceneGIFiltering.glsl
#include includes/CTASwizzling.glsl

uint2 GetSwizzledThreadID()
{
    return ThreadGroupTilingX
    (
        gl_NumWorkGroups.xy,
        uint2(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8),
        8u,
        gl_LocalInvocationID.xy,
        gl_WorkGroupID.xy
    );
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2   size = int2(pk_ScreenSize.xy);
    const int2   coord = int2(GetSwizzledThreadID());
    const float  depth = SampleMinZ(coord, 0);
    const float4 normalroughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalroughness.xyz;
    const float  roughness = normalroughness.w;
    const float  zbias = lerp(0.1f, 0.01f, -normal.z);

    GIDiff c_diff = GI_Load_Cur_Diff(coord);
    GISpec c_spec = GI_Load_Cur_Spec(coord);
    GIDiff s_diff = GI_Load_Diff(coord);
    GISpec s_spec = GI_Load_Spec(coord);

    const float wDiff = max(1.0f / (c_diff.history + 1.0f), PK_GI_MIN_ACCUM);
    const float wSpec = max(1.0f / (c_spec.history + 1.0f), PK_GI_MIN_ACCUM);
    const float maxGainDiff = PK_GI_MAX_LUMA_GAIN / (1.0f - wDiff);
    const float maxGainSpec = PK_GI_MAX_LUMA_GAIN / (1.0f - wSpec);
    const float2 lumaRangeDiff = float2(0.0f, SH_ToLuminanceL0(c_diff.sh) + maxGainDiff);
    const float2 lumaRangeSpec = float2(0.0f, dot(pk_Luminance.rgb, c_spec.radiance) + maxGainSpec);

    GI_SFLT_AntiFirefly(coord, normal, depth, zbias, roughness, lumaRangeDiff, lumaRangeSpec, s_diff, s_spec);

    // Interpolate samples
    c_diff.sh = SH_Interpolate(c_diff.sh, s_diff.sh, wDiff);
    c_diff.ao = lerp(c_diff.ao, s_diff.ao, wDiff);

#if PK_GI_APPROX_ROUGH_SPEC == 1
    if (roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        c_spec.radiance = lerp(c_spec.radiance, s_spec.radiance, wSpec);
        c_spec.ao = lerp(c_spec.ao, s_spec.ao, wSpec);
    }

    GI_Store_Diff(coord, c_diff);
    GI_Store_Spec(coord, c_spec);
}