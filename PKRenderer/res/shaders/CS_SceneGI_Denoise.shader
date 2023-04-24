#version 460

#multi_compile PASS_VARIANCE PASS_DISKBLUR

#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/Kernels.glsl

const float WAVELET_KERNEL[2][2] =
{
    { 1.0f, 0.5f  },
    { 0.5f, 0.25f }
};

struct WaveData
{
    SH diff;
    SH spec;
    uint history;
    float variance;
    float roughness;
    float depth;
    float3 normal;
};

struct FilterOutput
{
    SH diff;
    SH spec;
    float wDiff;
    float wSpec;
};

#define DENOISE_GROUP_SIZE_X 5
#define DENOISE_FILTER_RADIUS 1
const int SHARED_WIDTH = DENOISE_FILTER_RADIUS + DENOISE_GROUP_SIZE_X + DENOISE_FILTER_RADIUS;
shared WaveData waveData[SHARED_WIDTH][SHARED_WIDTH];

#if defined(PASS_VARIANCE)
    #define FILTER_PREPROCESS   \
        PreloadWaveData();      \
        barrier();              \

    #define FILTER_PASS FilterSVGF
    #define GET_WAVE_DATA(xx, yy) waveData[int(gl_LocalInvocationID.y) + DENOISE_FILTER_RADIUS + yy][int(gl_LocalInvocationID.x) + DENOISE_FILTER_RADIUS + xx]
#else
    #define FILTER_PREPROCESS

    #define FILTER_PASS FilterEmpty
    #define GET_WAVE_DATA(xx, yy) LoadWaveData(int2(gl_GlobalInvocationID.xy) + int2(xx,yy))
#endif


WaveData LoadWaveData(const int2 coord)
{
    WaveData d;
    d.history = 0xFFFFFFFFu;

    if (!All_InArea(coord, int2(0), pk_ScreenSize.xy))
    {
        return d;
    }

    d.depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(d.depth))
    {
        return d;
    }

    d.diff = SampleGI_SH(coord, PK_GI_DIFF_LVL);
    d.spec = SampleGI_SH(coord, PK_GI_SPEC_LVL);

    const float4 nr = SampleWorldNormalRoughness(coord);
    d.normal = nr.xyz;
    d.roughness = nr.w;
    
    const SceneGIMeta meta = SampleGI_Meta(coord);
    d.variance = meta.moments.y - pow2(meta.moments.x);
    d.history = meta.history;
    return d;
}

void FillWaveData(const int2 baseCoord, const int sharedOffset)
{
    const int2 sharedCoord = int2(sharedOffset % SHARED_WIDTH, sharedOffset / SHARED_WIDTH);
    waveData[sharedCoord.y][sharedCoord.x] = LoadWaveData(baseCoord + sharedCoord);
}

void PreloadWaveData()
{
    const int2 globalBasePix = int2(gl_WorkGroupID.xy) * DENOISE_GROUP_SIZE_X - int2(DENOISE_FILTER_RADIUS);
    const int threadIndex = int(gl_LocalInvocationIndex);

    const int sharedCount = SHARED_WIDTH * SHARED_WIDTH;
    const int threadCount = DENOISE_GROUP_SIZE_X * DENOISE_GROUP_SIZE_X;

    // how many threads should load only one pixel
    const int oneLoadCount = 2 * threadCount - sharedCount;

    if (threadIndex < oneLoadCount)
    {
        // first threads need to preload only 1 pixel
        FillWaveData(globalBasePix, threadIndex);
    }
    else
    {
        // now threads are loading 2 neighboring pixels
        const int neighborsIndex = threadIndex - oneLoadCount;
        FillWaveData(globalBasePix, oneLoadCount + neighborsIndex * 2 + 0);
        FillWaveData(globalBasePix, oneLoadCount + neighborsIndex * 2 + 1);
    }
}

float PrefilterLuminanceVariance()
{
    const float gaussianKernel[2][2] =
    {
        { 1.0 / 4.0, 1.0 / 8.0  },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };

    float r = 0;

    for (int yy = -DENOISE_FILTER_RADIUS; yy <= DENOISE_FILTER_RADIUS; yy++)
    for (int xx = -DENOISE_FILTER_RADIUS; xx <= DENOISE_FILTER_RADIUS; xx++)
    {
        WaveData w = GET_WAVE_DATA(xx, yy);

        if (w.history != 0xFFFFFFFFu)
        {
            r += GET_WAVE_DATA(xx, yy).variance * gaussianKernel[abs(xx)][abs(yy)];
        }
    }

    return sqrt(max(r, 0.0));
}


void FilterEmpty(in WaveData c, inout FilterOutput o) {}

void FilterSVGF(in WaveData c, inout FilterOutput o)
{
    const float c_l = SHToLuminance(c.diff, c.normal);
    const float variance = PrefilterLuminanceVariance();

    const float wLumMult = 1.0 / (4.0f * variance + 1e-4f);
    const float wRoughnessMult = saturate(c.roughness * 30);
    const float normalPower = clamp((c.history + 1), 1, 256);

    for (int x = -DENOISE_FILTER_RADIUS; x <= DENOISE_FILTER_RADIUS; ++x)
    for (int y = -DENOISE_FILTER_RADIUS; y <= DENOISE_FILTER_RADIUS; ++y)
    {
        if (x == 0 && y == 0)
        {
            continue;
        }

        const WaveData s = GET_WAVE_DATA(x, y);
    
        if (s.history >= 0xFFFFu)
        {
            continue;
        }
    
        const float s_nd = max(0.0f, dot(c.normal, s.normal));
        const float s_l = SHToLuminance(s.diff, c.normal);
        const float w_l = exp(-abs(c_l - s_l) * wLumMult);
        const float w_z = exp(-abs(c.depth - s.depth) / min(c.depth, s.depth));
        const float w_h = (s.history + 1.0) / 256.0f;
        const float w_n = pow(s_nd, normalPower);
        const float w_r = max(0.0, 1.0 - 10 * abs(c.roughness - s.roughness)) * wRoughnessMult;
        const float w_w = WAVELET_KERNEL[abs(y)][abs(x)];
        const float wBase = saturate(w_z * w_w * w_n * w_h);
        const float wDiff = wBase * w_l;
        const float wSpec = wBase * w_r;
   
        o.wDiff += wDiff;
        o.wSpec += wSpec;
        o.diff = AddSH(o.diff, s.diff, wDiff);
        o.spec = AddSH(o.spec, s.spec, wSpec);
    }
}

void FilterSparse3x3(in WaveData c, inout FilterOutput o)
{
    c.variance = sqrt(max(0.0f, c.variance));

    const float c_l = SHToLuminance(c.diff, c.normal);
    const float wLumMult = 1.0 / (4.0f * c.variance + 1e-4f);
    const float wRoughnessMult = saturate(c.roughness * 30);
    const float normalPower = clamp((c.history + 1) * 2, 1, 512);

    const int filterVScale = int(c.variance * 4.0f);
    const int filterScale0 = int(clamp(filterVScale + 1u, 1u, 4u));
    const int filterScale1 = int(clamp(filterVScale, 1u, 6u));

    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        if ((x == 0 && y == 0))
        {
            continue;
        }

        int2 offset = int2(x, y);
        offset *= x == 0 || y == 0 ? filterScale1 : filterScale0;
        const WaveData s = GET_WAVE_DATA(offset.x, offset.y);
    
        if (s.history >= 0xFFFFu)
        {
            continue;
        }
        
        const float s_nd = max(0.0f, dot(c.normal, s.normal));
        const float s_l = SHToLuminance(s.diff, c.normal);
        const float w_l = exp(-abs(c_l - s_l) * 16.0f);
        const float w_z = exp(-abs(c.depth - s.depth) / min(c.depth, s.depth));
        const float w_n = pow(s_nd, normalPower);
        const float w_r = max(0.0, 1.0 - 10 * abs(c.roughness - s.roughness)) * wRoughnessMult;
        const float wBase = saturate(w_z * w_n);
        const float wDiff = wBase * w_l;
        const float wSpec = wBase * w_r;
    
        o.wDiff += wDiff;
        o.wSpec += wSpec;
        o.diff = AddSH(o.diff, s.diff, wDiff);
        o.spec = AddSH(o.spec, s.spec, wSpec);
    }
}


layout(local_size_x = DENOISE_GROUP_SIZE_X, local_size_y = DENOISE_GROUP_SIZE_X, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    FILTER_PREPROCESS

    WaveData c = GET_WAVE_DATA(0, 0);

    if (c.history >= 0xFFFFu)
    {
        return;
    }

    FilterOutput o;
    o.wDiff = o.wSpec = 1.0f;
    o.diff = c.diff;
    o.spec = c.spec;

    FILTER_PASS(c, o);

    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(o.diff, 1.0f / o.wDiff));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(o.spec, 1.0f / o.wSpec));
}