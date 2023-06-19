#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

/*
void FilterBilateralDiff(const int2 coord, const int mip, inout GISampleDiff o)
{
    const int stride0 = 1 << (mip + 1);
    const int stride1 = stride0 / 2;
    const int2 base = (coord - stride1) / stride0;
    const float2 ddxy = float2(coord - stride1 - stride0 * base + 0.5f.xx) / stride0;

    GISampleDiff flt = pk_Zero_GISampleDiff;
    float wSum = 0.0f;

    const float bilinearWeights[4] =
    {
         (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) ,
         (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         ,
    };

    uint4 samples[4] =
    {
        GI_Load_Packed_Mip_SampleDiff(base + int2(0, 0), mip),
        GI_Load_Packed_Mip_SampleDiff(base + int2(1, 0), mip),
        GI_Load_Packed_Mip_SampleDiff(base + int2(0, 1), mip),
        GI_Load_Packed_Mip_SampleDiff(base + int2(1, 1), mip)
    };

    float4 zdiffs = float4
    (
        samples[0].w != 0u ? abs(o.depth - GI_Unpack_SampleDiff(samples[0]).depth) : PK_HALF_MAX,
        samples[1].w != 0u ? abs(o.depth - GI_Unpack_SampleDiff(samples[1]).depth) : PK_HALF_MAX,
        samples[2].w != 0u ? abs(o.depth - GI_Unpack_SampleDiff(samples[2]).depth) : PK_HALF_MAX,
        samples[3].w != 0u ? abs(o.depth - GI_Unpack_SampleDiff(samples[3]).depth) : PK_HALF_MAX
    );

    const float depthThreshold = 1e-1f * o.depth;

    if (All_Less(zdiffs, depthThreshold.xxxx))
    {
        for (uint i = 0; i < 4u; ++i)
        {
            const GISampleDiff s = GI_Unpack_SampleDiff(samples[i]);
            const float w_b = bilinearWeights[i];
            flt.sh = SHAdd(flt.sh, s.sh, w_b);
            flt.ao += s.ao * w_b;
            flt.depth += s.depth * w_b;
        }

        o = flt;
        return;
    }

    uint idx = 0u;
    float mindiff = zdiffs.x;

    for (uint i = 1u; i < 4u; ++i)
    {
        if (zdiffs[i] < mindiff)
        {
            idx = i;
            mindiff = zdiffs[i];
        }
    }

    o = GI_Unpack_SampleDiff(samples[idx]);
}

*/

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

    GISampleMeta meta = GI_Load_SampleMeta(coord);
    const int mip = 3 - int(meta.historyDiff);

    if (mip < 0)
    {
        return;
    }

    GISampleDiff diff = pk_Zero_GISampleDiff;
    GISampleSpec spec = pk_Zero_GISampleSpec;
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
        const uint4 p_diff = GI_Load_Packed_Mip_SampleDiff(base + int2(xx, yy), mip);
        const uint2 p_spec = GI_Load_Packed_Mip_SampleSpec(base + int2(xx, yy), mip);

        if (p_diff.w == 0u)
        {
            continue;
        }

        const GISampleDiff s_diff = GI_Unpack_SampleDiff(p_diff);
        const GISampleSpec s_spec = GI_Unpack_SampleSpec(p_spec);

        const float w_z = exp(-abs(depth - s_diff.depth) / depth);
        const float w_b = bilinearWeights[yy][xx];
        const float w = w_b * w_z;

        if (w > 1e-4f)
        {
            diff.sh = SHAdd(diff.sh, s_diff.sh, w);
            diff.ao += s_diff.ao * w;
            diff.depth += s_diff.depth * w;
            spec.radiance += s_spec.radiance * w;
            spec.ao += s_spec.ao * w;
            spec.depth += s_spec.depth * w;
            wSum += w;
        }
    }

    if (!isnan(wSum) && wSum > 1e-3f)
    {
        diff.sh = SHScale(diff.sh, 1.0f / wSum);
        diff.ao /= wSum;
        diff.depth /= wSum;
        spec.radiance /= wSum;
        spec.ao /= wSum;
        spec.depth /= wSum;

        GI_Store_SampleDiff(coord, diff);
        GI_Store_SampleSpec(coord, spec);
    }
}
