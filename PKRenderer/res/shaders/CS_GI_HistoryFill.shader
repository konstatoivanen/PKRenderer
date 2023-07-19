#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float depth = SampleMinZ(coord, 0);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    GIDiff c_diff = GI_Load_Diff(coord);
    GISpec c_spec = GI_Load_Spec(coord);

    const int mip = 3 - int(min(c_diff.history, c_spec.history));

    if (mip < 0)
    {
        return;
    }

    const bool skip_diff = c_diff.history > 3;
    const bool skip_spec = c_spec.history > 3;

    GIDiff diff = pk_Zero_GIDiff;
    GISpec spec = pk_Zero_GISpec;
    diff.history = c_diff.history;
    spec.history = c_spec.history;

    float wSum = 0.0f;

    const int stride0 = 1 << (mip + 1);
    const int stride1 = stride0 / 2;
    const int2 base = (coord - stride1) / stride0;
    const float2 ddxy = float2(coord - stride1 - stride0 * base + 0.5f.xx) / stride0;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };

    for (uint yy = 0; yy <= 1u; ++yy)
    for (uint xx = 0; xx <= 1u; ++xx)
    {
        const uint4 p_diff = GI_Load_Packed_Mip_Diff(base + int2(xx, yy), mip);
        const uint2 p_spec = GI_Load_Packed_Mip_Spec(base + int2(xx, yy), mip);
        const GIDiff s_diff = GI_Unpack_Diff(p_diff);
        const GISpec s_spec = GI_Unpack_Spec(p_spec);
        
        const float s_depth = SampleAvgZ(base + int2(xx, yy), mip + 1);

        const float w_z = 1.0f / (1e-4f + abs(depth - s_depth));
        const float w_b = bilinearWeights[yy][xx];
        const float w = w_b * w_z;

        if (p_diff.w == 0u || w > 1e-6f)
        {
            wSum += w;
            diff = GI_Sum_NoHistory(diff, s_diff, w);
            spec = GI_Sum_NoHistory(spec, s_spec, w);
        }
    }

    if (!isnan(wSum) && wSum > 1e-6f)
    {
        diff = GI_Mul_NoHistory(diff, 1.0f / wSum);
        spec = GI_Mul_NoHistory(spec, 1.0f / wSum);
        if (!skip_diff) GI_Store_Diff(coord, diff);
        if (!skip_spec) GI_Store_Spec(coord, spec);
    }
}
