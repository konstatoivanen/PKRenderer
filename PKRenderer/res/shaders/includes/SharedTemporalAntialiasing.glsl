#pragma once
#include Common.glsl

/*
Define these as TAA input textures
Due to shader compiler issues they will be optimized out if passed as parameters.
#define SAMPLE_TAA_SOURCE(uv)
#define SAMPLE_TAA_HISTORY(uv)
*/

struct TAADescriptor
{
    float2 texelSize;
    float2 jitter;
    float2 texcoord;
    float2 motion;
    float sharpness;
    float blendingStatic;
    float blendingMotion;
    float motionAmplification;
};

struct TAAOutput
{
    float3 color;
    float3 history;
};

float3 ClipColorToAABB(float3 color, float3 minimum, float3 maximum)
{
    float3 center = 0.5 * (maximum + minimum);
    float3 extents = 0.5 * (maximum - minimum);
    float3 offset = color - center;

    float3 ts = abs(extents / (offset + 0.0001));

    float t = saturate(min(min(ts.x, ts.y), ts.z));
    
    color = center + offset * t;
    
    return color;
}

TAAOutput SolveTemporalAntiAliasing(TAADescriptor desc)
{
    float2 uv = saturate(desc.texcoord - desc.jitter);

    float3 color = SAMPLE_TAA_SOURCE(uv);
    float3 color11 = SAMPLE_TAA_SOURCE(saturate(uv - desc.texelSize * 0.5f));
    float3 color00 = SAMPLE_TAA_SOURCE(saturate(uv + desc.texelSize * 0.5f));
    float3 corners = 4.0f * (color11 + color00) - 2.0f * color;

    color += (color - (corners * 0.166667f)) * 2.718282f * desc.sharpness;
    color = clamp(color, 0.0, PK_HALF_MAX_MINUS1);
    
    float3 average = (corners + color) * 0.142857f;
    float2 luminance = float2(dot(average, pk_Luminance.rgb), dot(color, pk_Luminance.rgb));

    float motionLength = length(desc.motion);
    float colorOffset = lerp(4.0f, 0.25f, saturate(motionLength * 100.0f)) * abs(luminance.x - luminance.y);
    
    float3 minimum = min(color00, color11) - colorOffset;
    float3 maximum = max(color00, color11) + colorOffset;

    float3 history = SAMPLE_TAA_HISTORY(saturate(desc.texcoord - desc.motion));
    history = ClipColorToAABB(history, minimum.xyz, maximum.xyz);

    float weight = clamp(lerp(desc.blendingStatic, desc.blendingMotion, motionLength * desc.motionAmplification), desc.blendingMotion, desc.blendingStatic);

    color = lerp(color, history, weight);
    color = clamp(color, 0.0, PK_HALF_MAX_MINUS1);

    TAAOutput o;
    o.color = color; 
    o.history = color;
    return o;
}