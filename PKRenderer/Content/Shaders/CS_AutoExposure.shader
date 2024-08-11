
#pragma pk_multi_compile PASS_HISTOGRAM PASS_AVG
#pragma pk_program SHADER_STAGE_COMPUTE HistogramCs PASS_HISTOGRAM
#pragma pk_program SHADER_STAGE_COMPUTE AverageCs PASS_AVG

#include "includes/Common.glsl"
#include "includes/PostFXResources.glsl"
#include "includes/PostFXAutoExposure.glsl"

#define HISTOGRAM_THREAD_COUNT 16
#define NUM_HISTOGRAM_BINS 256
#define DEFAULT_LOG_LUMINANCE_MIN -8.0f

// Source texture
PK_DECLARE_SET_PASS uniform sampler2D pk_Texture;

// Source: http://www.alextardif.com/HistogramLuminance.html
shared uint lds_Histogram[NUM_HISTOGRAM_BINS];

void SetAutoExposure(float exposure) { PK_BUFFER_DATA(pk_AutoExposure_Histogram, 256) = floatBitsToUint(exposure); }
void SetLogLuminanceMin(float value) { PK_BUFFER_DATA(pk_AutoExposure_Histogram, 257) = floatBitsToUint(value); }
float GetLogLuminanceMin() { return ReplaceIfResized(uintBitsToFloat(PK_BUFFER_DATA(pk_AutoExposure_Histogram, 257)), DEFAULT_LOG_LUMINANCE_MIN); }

layout(local_size_x = HISTOGRAM_THREAD_COUNT, local_size_y = HISTOGRAM_THREAD_COUNT, local_size_z = 1) in;

void HistogramCs()
{
    lds_Histogram[gl_LocalInvocationIndex] = 0;
    barrier();

    const float2 size = textureSize(pk_Texture, 0).xy >> 1;
    const float2 uv = (gl_GlobalInvocationID.xy + 0.5f) / size;
    
    const float3 color = texture(pk_Texture, uv).rgb;
    
    const float logMin = GetLogLuminanceMin();
    const float logRangeInv = 1.0f / pk_AutoExposure_LogLumaRange;
    const float luma = dot(PK_LUMA_BT709, color);
    const float lumaLog = saturate((log2(luma) - logMin) * logRangeInv);
    
    const uint binIndex = luma < 1e-4f ? 0u : uint(lumaLog * 254.0f + 1.0f);

    atomicAdd(lds_Histogram[binIndex], 1);

    barrier();
    atomicAdd(PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex), lds_Histogram[gl_LocalInvocationIndex]);
}

void AverageCs()
{
    uint countForThisBin = PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex);
    lds_Histogram[gl_LocalInvocationIndex] = countForThisBin * gl_LocalInvocationIndex;

    PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex) = 0;

    barrier();

    for (uint histogramSampleIndex = (NUM_HISTOGRAM_BINS >> 1); histogramSampleIndex > 0; histogramSampleIndex >>= 1)
    {
        if (gl_LocalInvocationIndex < histogramSampleIndex)
        {
            lds_Histogram[gl_LocalInvocationIndex] += lds_Histogram[gl_LocalInvocationIndex + histogramSampleIndex];
        }

        barrier();
    }

    if (gl_LocalInvocationIndex == 0)
    {
        float logMin = GetLogLuminanceMin();
        float logRange = pk_AutoExposure_LogLumaRange;

        const float2 size = textureSize(pk_Texture, 0).xy >> 1;
        const float numpx = size.x * size.y;
        const float weightedLumaLogAverage = (lds_Histogram[0] / max(numpx - countForThisBin, 1.0f)) - 1.0f;
        const float weightedLumaAverage = exp2((weightedLumaLogAverage / 254.0) * logRange + logMin);

        const float interpolant = ReplaceIfResized(pk_DeltaTime.x * pk_AutoExposure_Speed, 0.0f);

        // Source: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v2.pdf
        // Simplified factor for luminance to exposure.
        const float exposureCurrent = GetAutoExposure();
        const float exposureTarget = clamp(pk_AutoExposure_Target * (0.104167f / weightedLumaAverage), pk_AutoExposure_Min, pk_AutoExposure_Max);
        const float exposure = lerp_sat(exposureCurrent, exposureTarget, interpolant);

        const float biasToCenter = (floor(weightedLumaLogAverage) - 128.0f) / 255.0f;
        logMin += (biasToCenter / logRange) * float(abs(biasToCenter) > 0.1f);

        SetLogLuminanceMin(logMin);
        SetAutoExposure(exposure);
    }
}
