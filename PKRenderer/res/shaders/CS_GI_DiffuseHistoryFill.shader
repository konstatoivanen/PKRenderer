#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

// @ TODO Testing 
/*
    Test separate mip chain tex generated in accumulate kernel
    Implement spec filtering as well

    look at mipvis.pdn
*/

#define GROUP_SIZE 8u
#define LDS_SIZE 32u
shared uint4 shared_Samples[LDS_SIZE][LDS_SIZE];
shared uint shared_DoCompute;

void AddSample(inout GISampleDiff o, const GISampleDiff s, float w)
{
    o.sh = SHAdd(o.sh, s.sh, w);
    o.ao += s.ao * w;
    o.depth += s.depth * w;
}

uint4 NormalizeSample(GISampleDiff o, float w)
{
    if (w <= 1e-4f)
    {
        return uint4(0);
    }

    o.sh = SHScale(o.sh, 1.0f / w);
    o.ao /= w;
    o.depth /= w;
    return GI_Pack_SampleDiff(o);
}

void Group_LoadFirstMip(const int2 size)
{
    const int2 baseCoord = int(GROUP_SIZE) * 2 * (int2(gl_WorkGroupID.xy) / 2) - int(GROUP_SIZE).xx;
    const uint threadIndex = gl_LocalInvocationIndex;

    for (uint i = 0u; i < 16u; ++i)
    {
        const uint sharedOffset = 16u * threadIndex + i;
        const uint2 sharedCoord = uint2(sharedOffset % LDS_SIZE, sharedOffset / LDS_SIZE);
        const int2 coord = baseCoord + int2(sharedCoord);

        if (!All_InArea(coord, int2(0), size))
        {
            shared_Samples[sharedCoord.y][sharedCoord.x] = uint4(0);
            continue;
        }

        const uint4 packed = GI_Load_Packed_SampleDiff(coord);
        shared_Samples[sharedCoord.y][sharedCoord.x] = packed.w == 0u ? uint4(0) : packed;
    }
}

void Group_ReduceMip(const uint mip)
{
    const uint sharedWidth = LDS_SIZE >> mip;
    const uint sharedCount = sharedWidth * sharedWidth;
    const uint threadCount = GROUP_SIZE * GROUP_SIZE;
    const uint threadIndex = gl_LocalInvocationIndex;
    const uint loadCount = max(1u, sharedCount / threadCount);
    const uint strideStore = 1u << mip;
    const uint strideLoad = 1u << (mip - 1u);

    if (threadIndex < sharedCount)
    {
        for (uint i = 0u; i < loadCount; ++i)
        {
            const uint sharedOffset = loadCount * threadIndex + i;
            const uint2 sharedCoord = strideStore * uint2(sharedOffset % sharedWidth, sharedOffset / sharedWidth);

            GISampleDiff flt = pk_Zero_GISampleDiff;
            float wSum = 0.0f;

            for (uint xx = 0u; xx <= 1u; ++xx)
            for (uint yy = 0u; yy <= 1u; ++yy)
            {
                const uint2 xy = sharedCoord + strideLoad * uint2(xx, yy);
                const uint4 packed = shared_Samples[xy.y][xy.x];

                if (packed.w != 0u)
                {
                    AddSample(flt, GI_Unpack_SampleDiff(packed), 1.0f);
                    wSum += 1.0f;
                }
            }

            shared_Samples[sharedCoord.y][sharedCoord.x] = NormalizeSample(flt, wSum);
        }
    }
}

void FilterBilateral(inout GISampleDiff o, const uint mip)
{
    const uint2 sharedBase = (gl_WorkGroupID.xy % 2u) * GROUP_SIZE + GROUP_SIZE;

    const uint stride0 = 1u << mip;
    const uint stride1 = stride0 / 2u;

    const uint2 coord = sharedBase + gl_LocalInvocationID.xy;
    const uint2 base = stride0 * ((coord - stride1) / stride0);

    const float2 ddxy = float2(coord - base - stride1 + 0.5f.xx) / stride0;

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
        const uint4 packed = shared_Samples[base.y + yy * stride0][base.x + xx * stride0];

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
            AddSample(flt, s, weight);
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

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float depth = SampleLinearDepth(coord);
    const bool isValid = !Any_GEqual(coord, size) && Test_DepthFar(depth);

    GISampleFull filtered = GI_Load_SampleFull(coord);
    
    const uint history = uint(filtered.meta.historyDiff) / 2;

    shared_DoCompute = 0u;
    barrier();
    atomicMax(shared_DoCompute, isValid && history <= 4 ? 1u : 0u);
    barrier();

    if (shared_DoCompute == 0)
    {
        GI_Store_SampleFull(coord, filtered);
        return;
    }

    Group_LoadFirstMip(size);
    barrier();

    if (history == 4u)
    {
        FilterBilateral(filtered.diff, 0);
    }
    
    Group_ReduceMip(1);
    barrier();

    if (history == 3u)
    {
        FilterBilateral(filtered.diff, 1);
    }

    Group_ReduceMip(2);
    barrier();

    if (history == 2u)
    {
        FilterBilateral(filtered.diff, 2);
    }

    Group_ReduceMip(3);
    barrier();

    if (history == 1u)
    {
        FilterBilateral(filtered.diff, 3);
    }

    Group_ReduceMip(4);
    barrier();

    if (history == 0u)
    {
        FilterBilateral(filtered.diff, 4);
    }

    if (isValid)
    {
        GI_Store_SampleFull(coord, filtered);
    }
}