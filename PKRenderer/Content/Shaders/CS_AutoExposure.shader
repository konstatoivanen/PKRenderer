
#pragma pk_multi_compile PASS_HISTOGRAM PASS_AVG
#pragma pk_program SHADER_STAGE_COMPUTE HistogramCs PASS_HISTOGRAM
#pragma pk_program SHADER_STAGE_COMPUTE AverageCs PASS_AVG

#include "includes/Common.glsl"
#include "includes/PostFXResources.glsl"
#include "includes/PostFXAutoExposure.glsl"

#define HISTOGRAM_THREAD_COUNT 16
#define NUM_HISTOGRAM_BINS 256

#define LOG_LUMINANCE_MIN pk_AutoExposure_MinLogLuma
#define LOG_LUMINANCE_INV_RANGE pk_AutoExposure_InvLogLumaRange
#define LOG_LUMINANCE_RANGE pk_AutoExposure_LogLumaRange
#define TARGET_EXPOSURE pk_AutoExposure_Target
#define EXPOSURE_ADJUST_SPEED pk_AutoExposure_Speed

// Source texture
PK_DECLARE_SET_DRAW uniform texture2D pk_Texture;

// Source: http://www.alextardif.com/HistogramLuminance.html
shared uint HistogramShared[NUM_HISTOGRAM_BINS];

void SetAutoExposure(float exposure)
{
    PK_BUFFER_DATA(pk_AutoExposure_Histogram, 256) = floatBitsToUint(exposure);
}

// Source: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v2.pdf
float ComputeEV100(float aperture, float shutterTime, float ISO) { return log2(pow2(aperture) / shutterTime * 100 / ISO); }
float ComputeEV100FromAvgLuminance(float avgLuminance) { return log2(avgLuminance * 100.0f / 12.5f); }
float ConvertEV100ToExposure(float EV100) { return 1.0f / (1.2f * pow(2.0f, EV100)); }

uint HDRToHistogramBin(float3 hdrColor)
{
    float luminance = dot(PK_LUMA_BT709, hdrColor);

    if (luminance < 1e-4f)
    {
        return 0;
    }

    float logLuminance = saturate((log2(luminance) - LOG_LUMINANCE_MIN) * LOG_LUMINANCE_INV_RANGE);

    return uint(logLuminance * 254.0 + 1.0);
}

layout(local_size_x = HISTOGRAM_THREAD_COUNT, local_size_y = HISTOGRAM_THREAD_COUNT, local_size_z = 1) in;

void HistogramCs()
{
    HistogramShared[gl_LocalInvocationIndex] = 0;
    barrier();

    float2 size = textureSize(pk_Texture, 0).xy;

    if (gl_GlobalInvocationID.x < size.x && gl_GlobalInvocationID.y < size.y)
    {
        // @TODO normalize this. Currently this uses bloom layer 2, due to half res perf benefits, which is is not scaled down from upsampling. manual rescale here :/
        float3 hdrColor = texelFetch(pk_Texture, int2(gl_GlobalInvocationID.xy), 0).xyz / 8.0f;

        uint binIndex = HDRToHistogramBin(hdrColor);
        atomicAdd(HistogramShared[binIndex], 1);
    }

    barrier();
    atomicAdd(PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex), HistogramShared[gl_LocalInvocationIndex]);
}

void AverageCs()
{
    uint countForThisBin = PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex);
    HistogramShared[gl_LocalInvocationIndex] = countForThisBin * gl_LocalInvocationIndex;

    PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex) = 0;

    barrier();

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
        const float2 size = textureSize(pk_Texture, 0).xy;
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
}
