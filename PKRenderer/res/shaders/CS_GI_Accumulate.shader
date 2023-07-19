#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
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
    GIDiff f_diff = GI_Load_Diff(coord);
    GISpec c_spec = GI_Load_Cur_Spec(coord);
    GISpec f_spec = GI_Load_Spec(coord);

    
    float wSumDiff = 1.0f;
    float wSumSpec = 1.0f;
    float lumaDiff = SH_ToLuminanceL0(f_diff.sh);
    float lumaSpec = dot(pk_Luminance.rgb, f_spec.radiance);

    const float k_R0 = 1.0f / lerp(0.01f, 1.0f, roughness);
    const float k_R1 = -roughness * k_R0;

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
        const float w_r = exp(-abs(s_nr.w * k_R0 + k_R1));

        const float w_diff = w_n * w_d;
        const float w_spec = pow5(w_n) * w_d * w_r;
        const bool isValid = Test_InScreen(xy) && Test_DepthReproject(depth, s_depth, zbias);

        if (isValid && !isnan(w_diff) && w_diff > 1e-4f)
        {
            wSumDiff += w_diff;
            lumaDiff += s_lumaDiff * w_diff;
            f_diff = GI_Sum(f_diff, s_diff, w_diff);
        }

        if (isValid && !isnan(w_spec) && w_spec > 1e-4f)
        {
            wSumSpec += w_spec;
            lumaSpec += s_lumaSpec * w_spec;
            f_spec = GI_Sum(f_spec, s_spec, w_spec);
        }
    }

    const float wDiff = max(1.0f / (c_diff.history + 1.0f), PK_GI_MIN_ACCUM);
    const float wSpec = max(1.0f / (c_spec.history + 1.0f), PK_GI_MIN_ACCUM);

    const float maxGainDiff = PK_GI_MAX_LUMA_GAIN / (1.0f - wDiff);
    const float maxGainSpec = PK_GI_MAX_LUMA_GAIN / (1.0f - wSpec);

    lumaDiff = lumaDiff / wSumDiff + 1e-4f;
    lumaSpec = lumaSpec / wSumSpec + 1e-4f;
    const float scaleDiff = clamp(lumaDiff, 0.0f, SH_ToLuminanceL0(c_diff.sh) + maxGainDiff) / lumaDiff;
    const float scaleSpec = clamp(lumaSpec, 0.0f, dot(pk_Luminance.rgb, c_spec.radiance) + maxGainSpec) / lumaSpec;

    f_diff = GI_Mul_NoHistory(f_diff, scaleDiff / wSumDiff);
    f_spec = GI_Mul_NoHistory(f_spec, scaleSpec / wSumSpec);

    // Interpolate samples
    c_diff.sh = SH_Interpolate(c_diff.sh, f_diff.sh, wDiff);
    c_diff.ao = lerp(c_diff.ao, f_diff.ao, wDiff);

#if PK_GI_APPROX_ROUGH_SPEC == 1
    if (roughness < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        c_spec.radiance = lerp(c_spec.radiance, f_spec.radiance, wSpec);
        c_spec.ao = lerp(c_spec.ao, f_spec.ao, wSpec);
    }

    GI_Store_Diff(coord, c_diff);
    GI_Store_Spec(coord, c_spec);
}