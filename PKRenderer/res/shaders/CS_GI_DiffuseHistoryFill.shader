#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

void FilterBilateralDiff(const int2 coord, const int mip, inout GISampleDiff o)
{
    const int stride0 = 1 << (mip + 1);
    const int stride1 = stride0 / 2;
    const int2 base = (coord - stride1) / stride0;
    const float2 ddxy = float2(coord - stride1 - stride0 * base + 0.5f.xx) / stride0;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };

    GISampleDiff flt = pk_Zero_GISampleDiff;
    float wSum = 0.0f;

    for (uint yy = 0; yy <= 1u; ++yy)
    for (uint xx = 0; xx <= 1u; ++xx)
    {
        const uint4 packed = GI_Load_Packed_Mip_SampleDiff(base + int2(xx, yy), mip);

        if (packed.w == 0u)
        {
            continue;
        }

        const GISampleDiff s = GI_Unpack_SampleDiff(packed);
        const float w_z = pow2(exp(-abs(o.depth - s.depth) / max(o.depth, s.depth)));
        const float w_b = bilinearWeights[yy][xx];
        const float weight = w_b * w_z;
        
        if (weight > 1e-6f)
        {
            flt.sh = SHAdd(flt.sh, s.sh, weight);
            flt.ao += s.ao * weight;
            flt.depth += s.depth * weight;
            wSum += weight;
        }
    }

    if (wSum > 1e-6f)
    {
        o.sh = SHScale(flt.sh, 1.0f / wSum);
        o.ao /= wSum;
        o.depth /= wSum;
    }
}

layout(local_size_x = 16, local_size_y = 2, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    const float depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    GISampleMeta meta = GI_Load_SampleMeta(coord);
    const int sampleMip = 3 - int(meta.historyDiff) / 2;

    if (sampleMip < 0)
    {
        return;
    }

    GISampleDiff diff = GI_Load_SampleDiff(coord);
    FilterBilateralDiff(coord, sampleMip, diff);
    GI_Store_SampleDiff(coord, diff);
}