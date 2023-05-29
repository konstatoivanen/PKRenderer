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

#define GROUP_SIZE 8
#define LDS_SIZE 32
int lds_mip = 0;
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
    const int2 baseCoord = GROUP_SIZE * 2 * (int2(gl_WorkGroupID.xy) / 2) - int2(GROUP_SIZE);
    const int threadIndex = int(gl_LocalInvocationIndex);

    for (int i = 0; i < 16; ++i)
    {
        const int sharedOffset = 16 * threadIndex + i;
        const int2 sharedCoord = int2(sharedOffset % LDS_SIZE, sharedOffset / LDS_SIZE);
        const int2 coord = baseCoord + sharedCoord;

        if (!All_InArea(coord, int2(0), size))
        {
            shared_Samples[sharedCoord.y][sharedCoord.x] = uint4(0);
            continue;
        }

        const uint4 packed = GI_Load_Packed_SampleDiff(coord);

        if (packed.w == 0u)
        {
            shared_Samples[sharedCoord.y][sharedCoord.x] = uint4(0);
            continue;
        }

        shared_Samples[sharedCoord.y][sharedCoord.x] = packed;
    }
}

void Group_ReduceMip()
{
    lds_mip++;

    const int sharedWidth = LDS_SIZE >> lds_mip;
    const int sharedCount = sharedWidth * sharedWidth;
    const int threadCount = GROUP_SIZE * GROUP_SIZE;
    const int threadIndex = int(gl_LocalInvocationIndex);
    const int loadCount = max(1, sharedCount / threadCount);

    const int strideStore = 1 << (lds_mip);
    const int strideLoad = 1 << (lds_mip - 1);

    if (threadIndex >= sharedCount)
    {
        return;
    }

    for (int i = 0; i < loadCount; ++i)
    {
        const int sharedOffset = loadCount * threadIndex + i;
        const int2 sharedCoord = strideStore * int2(sharedOffset % sharedWidth, sharedOffset / sharedWidth);

        GISampleDiff flt = pk_Zero_GISampleDiff;
        float wSum = 0.0f;

        for (int xx = 0; xx <= 1; ++xx)
        for (int yy = 0; yy <= 1; ++yy)
        {
            const int2 xy = sharedCoord + strideLoad * int2(xx, yy);
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

void FilterBilateral(inout GISampleDiff o)
{
    const int2 sharedBase = (int2(gl_WorkGroupID.xy) % int2(2)) * GROUP_SIZE + GROUP_SIZE;

    const int stride0 = 1 << lds_mip;
    const int stride1 = stride0 / 2;

    const int2 coord = sharedBase + int2(gl_LocalInvocationID.xy);
    const int2 base = stride0 * ((coord - stride1) / stride0);

    const float2 ddxy = float2(coord - base - stride1 + 0.5f.xx) / stride0;
    const float zbias = o.depth * 1e-1f;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };

    GISampleDiff flt = pk_Zero_GISampleDiff;
    float wSum = 0.0f;

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
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
        FilterBilateral(filtered.diff);
    }
    
    Group_ReduceMip();
    barrier();

    if (history == 3u)
    {
        FilterBilateral(filtered.diff);
    }

    Group_ReduceMip();
    barrier();

    if (history == 2u)
    {
        FilterBilateral(filtered.diff);
    }

    Group_ReduceMip();
    barrier();

    if (history == 1u)
    {
        FilterBilateral(filtered.diff);
    }

    Group_ReduceMip();
    barrier();

    if (history == 0u)
    {
        FilterBilateral(filtered.diff);
    }

    if (isValid)
    {
        GI_Store_SampleFull(coord, filtered);
    }
}