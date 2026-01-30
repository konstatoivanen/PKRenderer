
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

[numthreads(HISTOGRAM_THREAD_COUNT, HISTOGRAM_THREAD_COUNT, 1u)]
void HistogramCs()
{
    lds_Histogram[gl_LocalInvocationIndex] = 0;
    barrier();

    const float2 resolution = textureSize(pk_Texture, 0).xy >> 1;
    const float2 uv = (gl_GlobalInvocationID.xy + 0.5f) / resolution;
    
    const float3 color = texture(pk_Texture, uv).rgb;
    
    const float luma_log_min = GetLogLuminanceMin();
    const float luma_log_range = pk_AutoExposure_LogLumaRange;

    const float luma = dot(PK_LUMA_BT709, color);
    const float luma_log = saturate((log2(luma) - luma_log_min) / luma_log_range);
    
    const uint bin_index = luma < 1e-4f ? 0u : uint(luma_log * 254.0f + 1.0f);

    atomicAdd(lds_Histogram[bin_index], 1);

    barrier();
    atomicAdd(PK_BUFFER_DATA(pk_AutoExposure_Histogram, gl_LocalInvocationIndex), lds_Histogram[gl_LocalInvocationIndex]);
}

[numthreads(HISTOGRAM_THREAD_COUNT, HISTOGRAM_THREAD_COUNT, 1u)]
void AverageCs()
{
    const uint thread = gl_LocalInvocationIndex;
    const uint local_bin_val = PK_BUFFER_DATA(pk_AutoExposure_Histogram, thread);

    lds_Histogram[thread] = local_bin_val * thread;

    PK_BUFFER_DATA(pk_AutoExposure_Histogram, thread) = 0;

    barrier();

    for (uint bin_index = (NUM_HISTOGRAM_BINS >> 1); bin_index > 0; bin_index >>= 1)
    {
        if (thread < bin_index)
        {
            lds_Histogram[thread] += lds_Histogram[thread + bin_index];
        }

        barrier();
    }

    if (thread == 0)
    {
        const float luma_log_min = GetLogLuminanceMin();
        const float luma_log_range = pk_AutoExposure_LogLumaRange;

        const float2 resolution = textureSize(pk_Texture, 0).xy >> 1;
        const float pixel_count = resolution.x * resolution.y;

        const float luma_log_avg = (lds_Histogram[0] / max(pixel_count - local_bin_val, 1.0f)) - 1.0f;
        const float luma_avg = exp2((luma_log_avg / 254.0) * luma_log_range + luma_log_min);

        const float interpolant = ReplaceIfResized(pk_DeltaTime.x * pk_AutoExposure_Speed, 0.0f);

        // Source: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v2.pdf
        // Simplified factor for luminance to exposure.
        const float exposure_current = GetAutoExposure();
        const float exposure_target = clamp(pk_AutoExposure_Target * (0.104167f / luma_avg), pk_AutoExposure_Min, pk_AutoExposure_Max);
        const float exposure_final = lerp_sat(exposure_current, exposure_target, interpolant);

        const float white_point_bias = (floor(luma_log_avg) - 128.0f) / 255.0f;
        const float luma_log_min_biased = luma_log_min + (white_point_bias / luma_log_range) * float(abs(white_point_bias) > 0.1f);

        SetLogLuminanceMin(luma_log_min_biased);
        SetAutoExposure(exposure_final);
    }
}
