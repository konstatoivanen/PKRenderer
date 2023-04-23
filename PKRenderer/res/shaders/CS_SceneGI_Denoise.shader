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

#define DENOISE_GROUP_SIZE_X 5
#define DENOISE_FILTER_RADIUS 1
const int SHARED_WIDTH = DENOISE_FILTER_RADIUS + DENOISE_GROUP_SIZE_X + DENOISE_FILTER_RADIUS;
shared WaveData waveData[SHARED_WIDTH][SHARED_WIDTH];

void FillWaveData(const int2 globalBasePix, const int sharedOffset)
{
    const int2 sharedPix = int2(sharedOffset % SHARED_WIDTH, sharedOffset / SHARED_WIDTH);
    const int2 globalPix = globalBasePix + sharedPix;
    WaveData data;

    const SceneGIMeta meta = SampleGI_Meta(globalPix);

    if (!meta.isOOB && All_GEqual(globalPix, int2(0)) && All_Less(globalPix, pk_ScreenSize.xy))
    {
        const float4 nr = SampleWorldNormalRoughness(globalPix);
        data.diff = SampleGI_SH(globalPix, PK_GI_DIFF_LVL);
        data.spec = SampleGI_SH(globalPix, PK_GI_SPEC_LVL);
        data.depth = SampleLinearDepth(globalPix);
        data.normal = nr.xyz;
        data.roughness = nr.w;
        data.variance = (meta.moments.y - pow2(meta.moments.x));
        data.history = meta.history;
    }
    else
    {
        data.history = 0xFFFFu;
    }

    waveData[sharedPix.y][sharedPix.x] = data;
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

#define GET_WAVE_DATA(xx, yy) waveData[int(gl_LocalInvocationID.y) + DENOISE_FILTER_RADIUS + yy][int(gl_LocalInvocationID.x) + DENOISE_FILTER_RADIUS + xx]

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

void DenoiseSVGF(const int2 coord, const int2 size)
{
    SceneGIMeta meta = SampleGI_Meta(coord);

    if (meta.isOOB)
    {
        return;
    }

    const WaveData c = GET_WAVE_DATA(0, 0);

    float2 W = c.history == 0u ? 0.125f.xx : 1.0f.xx;
    SH diffSH = ScaleSH(c.diff, W.x);
    SH specSH = ScaleSH(c.spec, W.y);

    const float c_l = SHToLuminance(c.diff, c.normal);
    const float variance = PrefilterLuminanceVariance();

    const float wLumMult = 1.0 / (4.0f * variance + 1e-4f);
    const float wRoughnessMult = saturate(c.roughness * 30);
    const float normalPower = clamp((c.history + 1) * 16, 1, 128);

    for (int x = -DENOISE_FILTER_RADIUS; x <= DENOISE_FILTER_RADIUS; ++x)
    for (int y = -DENOISE_FILTER_RADIUS; y <= DENOISE_FILTER_RADIUS; ++y)
    {
        int2 offset = int2(x, y);
        const int2 scoord = coord + offset;
    
        if ((x == 0 && y == 0) || Any_Less(scoord, int2(0)) || Any_GEqual(scoord, size))
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
        const float w_z = exp(-abs(c.depth - s.depth) / max(c.depth, s.depth));
        const float w_h = s.history == 0u ? 0.125f : 1.0f;
        const float w_n = pow(s_nd, normalPower);
        const float w_r = max(0.0, 1.0 - 10 * abs(c.roughness - s.roughness)) * wRoughnessMult;
        const float w_w = WAVELET_KERNEL[abs(y)][abs(x)];
        const float wBase = saturate(w_h * w_z * w_w * w_n);
        const float wDiff = wBase * w_l;
        const float wSpec = wBase * w_r;
   
        W += float2(wDiff, wSpec);
        diffSH = AddSH(diffSH, s.diff, wDiff);
        specSH = AddSH(specSH, s.spec, wSpec);
    }

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(diffSH, 1.0f / W.x));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(specSH, 1.0f / W.y));
}

void DenoiseSparse3x3(const int2 coord, const int2 size)
{
    SceneGIMeta meta = SampleGI_Meta(coord);

    if (meta.isOOB)
    {
        return;
    }

    WaveData c;
    const float4 cnr = SampleWorldNormalRoughness(coord);
    c.diff = SampleGI_SH(coord, PK_GI_DIFF_LVL);
    c.spec = SampleGI_SH(coord, PK_GI_SPEC_LVL);
    c.depth = SampleLinearDepth(coord);
    c.normal = cnr.xyz;
    c.roughness = cnr.w;
    c.variance = sqrt(max(0.0f, meta.moments.y - pow2(meta.moments.x)));
    c.history = meta.history;

    float2 W = c.history == 0u ? 0.125f.xx : 1.0f.xx;
    SH diffSH = ScaleSH(c.diff, W.x);
    SH specSH = ScaleSH(c.spec, W.y);

    const float c_l = SHToLuminance(c.diff, c.normal);

    const float wLumMult = 1.0 / (4.0f * c.variance + 1e-4f);
    const float wRoughnessMult = saturate(c.roughness * 30);
    const float normalPower = clamp((c.history + 1) * 16, 1, 256);

    const int filterVScale = int(max(1.0f / (c.history + 1.0f), c.variance) * 8.0f);
    const int filterScale0 = int(clamp(filterVScale + 1u, 1u, 4u));
    const int filterScale1 = int(clamp(filterVScale, 1u, 6u));

    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        int2 offset = int2(x, y);
        offset *= x == 0 || y == 0 ? filterScale1 : filterScale0;
    
        const int2 scoord = coord + offset;
    
        if ((x == 0 && y == 0) || Any_Less(scoord, int2(0)) || Any_GEqual(scoord, size))
        {
            continue;
        }
    
        const SceneGIMeta sampleMeta = SampleGI_Meta(scoord);
    
        if (sampleMeta.isOOB)
        {
            continue;
        }
        
        WaveData s;
        const float4 snr = SampleWorldNormalRoughness(scoord);
        s.diff = SampleGI_SH(scoord, PK_GI_DIFF_LVL);
        s.spec = SampleGI_SH(scoord, PK_GI_SPEC_LVL);
        s.depth = SampleLinearDepth(scoord);
        s.normal = snr.xyz;
        s.roughness = snr.w;
        s.history = sampleMeta.history;
    
        const float s_nd = max(0.0f, dot(c.normal, s.normal));
        const float s_l = SHToLuminance(s.diff, c.normal);
        const float w_l = exp(-abs(c_l - s_l) * 16.0f);
        const float w_z = exp(-abs(c.depth - s.depth) / max(c.depth, s.depth));
        const float w_h = s.history == 0u ? 0.125f : 1.0f;
        const float w_n = pow(s_nd, normalPower);
        const float w_r = max(0.0, 1.0 - 10 * abs(c.roughness - s.roughness)) * wRoughnessMult;
        const float wBase = saturate(w_h * w_z * w_n);
        const float wDiff = wBase * w_l;
        const float wSpec = wBase * w_r;
    
        W += float2(wDiff, wSpec);
        diffSH = AddSH(diffSH, s.diff, wDiff);
        specSH = AddSH(specSH, s.spec, wSpec);
    }

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(diffSH, 1.0f / W.x));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(specSH, 1.0f / W.y));
}

layout(local_size_x = DENOISE_GROUP_SIZE_X, local_size_y = DENOISE_GROUP_SIZE_X, local_size_z = 1) in;
void main()
{
    int2 size = int2(pk_ScreenSize.xy);
    int2 coord = int2(gl_GlobalInvocationID.xy);

#if defined(PASS_VARIANCE)
    PreloadWaveData();
    barrier();

    if (Any_GEqual(coord, size))
    {
        return;
    }

    DenoiseSVGF(coord, size);
#else
    if (Any_GEqual(coord, size))
    {
        return;
    }

    DenoiseSparse3x3(coord, size);
#endif
}