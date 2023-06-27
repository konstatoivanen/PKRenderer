#version 450
#multi_compile PASS_HISTOGRAM PASS_AVG

#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedPostEffects.glsl
#include includes/SharedHistogram.glsl

#define HISTOGRAM_THREAD_COUNT 16
#define NUM_HISTOGRAM_BINS 256
#define EPSILON 0.0001

#define LOG_LUMINANCE_MIN pk_MinLogLuminance
#define LOG_LUMINANCE_INV_RANGE pk_InvLogLuminanceRange
#define LOG_LUMINANCE_RANGE pk_LogLuminanceRange
#define TARGET_EXPOSURE pk_TargetExposure
#define EXPOSURE_ADJUST_SPEED pk_AutoExposureSpeed

PK_DECLARE_SET_DRAW uniform sampler2D _MainTex;

// Source: http://www.alextardif.com/HistogramLuminance.html
shared uint HistogramShared[NUM_HISTOGRAM_BINS];

void SetAutoExposure(float exposure)
{
    PK_BUFFER_DATA(pk_Histogram, 256) = floatBitsToUint(exposure);
}

// Source: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v2.pdf
float ComputeEV100(float aperture, float shutterTime, float ISO)
{
    // EV number is defined as:
    // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
    // This gives
    // EV_s = log2 (N^2 / t)
    // EV_100 + log2 (S /100) = log2 (N^2 / t)
    // EV_100 = log2 (N^2 / t) - log2 (S /100)
    // EV_100 = log2 (N^2 / t . 100 / S)
    return log2(pow2(aperture) / shutterTime * 100 / ISO);
}

float ComputeEV100FromAvgLuminance(float avgLuminance)
{
    // We later use the middle gray at 12.7% in order to have
    // a middle gray at 18% with a sqrt (2) room for specular highlights
    // But here we deal with the spot meter measuring the middle gray
    // which is fixed at 12.5 for matching standard camera
    // constructor settings (i.e. calibration constant K = 12.5)
    // Reference : http://en.wikipedia.org/wiki/Film_speed
    return log2(avgLuminance * 100.0f / 12.5f);
}

float ConvertEV100ToExposure(float EV100)
{
    // Compute the maximum luminance possible with H_sbs sensitivity
    // maxLum = 78 / ( S * q ) * N^2 / t
    // = 78 / ( S * q ) * 2^ EV_100
    // = 78 / (100 * 0.65) * 2^ EV_100
    // = 1.2 * 2^ EV
    // Reference : http://en.wikipedia.org/wiki/Film_speed
    float maxLuminance = 1.2f * pow(2.0f, EV100);
    return 1.0f / maxLuminance;
}

uint HDRToHistogramBin(float3 hdrColor)
{
    float luminance = dot(pk_Luminance.rgb, hdrColor);

    if (luminance < EPSILON)
    {
        return 0;
    }

    float logLuminance = saturate((log2(luminance) - LOG_LUMINANCE_MIN) * LOG_LUMINANCE_INV_RANGE);

    return uint(logLuminance * 254.0 + 1.0);
}

layout(local_size_x = HISTOGRAM_THREAD_COUNT, local_size_y = HISTOGRAM_THREAD_COUNT, local_size_z = 1) in;
void main()
{
#if defined(PASS_HISTOGRAM)
    HistogramShared[gl_LocalInvocationIndex] = 0;
    barrier();

    float2 size = textureSize(_MainTex, 0).xy;

    if (gl_GlobalInvocationID.x < size.x && gl_GlobalInvocationID.y < size.y)
    {
        float3 hdrColor = texelFetch(_MainTex, int2(gl_GlobalInvocationID.xy), 0).xyz;
        uint binIndex = HDRToHistogramBin(hdrColor);
        atomicAdd(HistogramShared[binIndex], 1);
    }

    barrier();
    atomicAdd(PK_BUFFER_DATA(pk_Histogram, gl_LocalInvocationIndex), HistogramShared[gl_LocalInvocationIndex]);
#else
    uint countForThisBin = PK_BUFFER_DATA(pk_Histogram, gl_LocalInvocationIndex);
    HistogramShared[gl_LocalInvocationIndex] = countForThisBin * gl_LocalInvocationIndex;

    PK_BUFFER_DATA(pk_Histogram, gl_LocalInvocationIndex) = 0;

    barrier();

#pragma unroll
    for (uint histogramSampleIndex = (NUM_HISTOGRAM_BINS >> 1); histogramSampleIndex > 0; histogramSampleIndex >>= 1)
    {
        if (gl_LocalInvocationIndex < histogramSampleIndex)
        {
            HistogramShared[gl_LocalInvocationIndex] += HistogramShared[gl_LocalInvocationIndex + histogramSampleIndex];
        }

        barrier();
    }

    if (gl_LocalInvocationIndex == 0)
    {
        const float2 size = textureSize(_MainTex, 0).xy;
        const float numpx = size.x * size.y;
        const float weightedLogAverage = (HistogramShared[0] / max(numpx - countForThisBin, 1.0)) - 1.0;
        const float weightedAverageLuminance = exp2((weightedLogAverage / 254.0) * LOG_LUMINANCE_RANGE + LOG_LUMINANCE_MIN);
        const float EV100 = ComputeEV100FromAvgLuminance(weightedAverageLuminance);

        const float currentExposure = GetAutoExposure();
        const float targetExposure = TARGET_EXPOSURE * ConvertEV100ToExposure(EV100);
        const float interpolant = ReplaceIfResized(pk_DeltaTime.x * EXPOSURE_ADJUST_SPEED, 0.0f);
        const float exposure = lerp_sat(currentExposure, targetExposure, interpolant);

        SetAutoExposure(exposure);
    }
#endif
}