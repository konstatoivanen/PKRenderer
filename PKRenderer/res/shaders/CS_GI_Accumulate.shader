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
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(GetSwizzledThreadID());
    const float depth = SampleMinZ(coord, 0);

    GISampleDiff c_diff = GI_Load_Cur_SampleDiff(coord);
    GISampleSpec c_spec = GI_Load_Cur_SampleSpec(coord);
    GISampleDiff s_diff = GI_Load_SampleDiff(coord);
    GISampleSpec s_spec = GI_Load_SampleSpec(coord);

    const float wDiff = max(1.0f / (c_diff.history + 1.0f), 0.03f);
    const float wSpec = max(1.0f / (c_spec.history + 1.0f), 0.03f);

    // Interpolate samples
    c_diff.sh = SH_Interpolate(c_diff.sh, s_diff.sh, wDiff);
    c_diff.ao = lerp(c_diff.ao, s_diff.ao, wDiff);

    c_spec.radiance = lerp(c_spec.radiance, s_spec.radiance, wSpec);
    c_spec.ao = lerp(c_spec.ao, s_spec.ao, wSpec);

    GI_Store_SampleDiff(coord, c_diff);
    GI_Store_SampleSpec(coord, c_spec);
}