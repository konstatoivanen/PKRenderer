#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedPostEffects.glsl
#include includes/Encoding.glsl

PK_DECLARE_SET_DRAW uniform sampler2D _SourceTex;
PK_DECLARE_SET_DRAW uniform sampler2D _HistoryReadTex;
layout(r32ui, set = PK_SET_DRAW) uniform uimage2D _HistoryWriteTex;

#define SAMPLE_TAA_SOURCE(uv) tex2D(_SourceTex, uv).rgb
#define SAMPLE_TAA_HISTORY(uv) tex2D(_HistoryReadTex, uv).rgb

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

float3 SolveTemporalAntiAliasing(TAADescriptor desc)
{
    float2 uv = desc.texcoord - desc.jitter;

    float3 history = SAMPLE_TAA_HISTORY(desc.texcoord - desc.motion);
    float3 color = SAMPLE_TAA_SOURCE(uv);
    float3 color11 = SAMPLE_TAA_SOURCE(uv - desc.texelSize * 0.5f);
    float3 color00 = SAMPLE_TAA_SOURCE(uv + desc.texelSize * 0.5f);
    float3 corners = 4.0f * (color11 + color00) - 2.0f * color;

    color += (color - (corners * 0.166667f)) * 2.718282f * desc.sharpness;
    color = clamp(color, 0.0, PK_HALF_MAX_MINUS1);

    float3 average = (corners + color) * 0.142857f;
    float2 luminance = float2(dot(average, pk_Luminance.rgb), dot(color, pk_Luminance.rgb));

    float motionLength = length(desc.motion);
    float colorOffset = lerp(4.0f, 0.25f, saturate(motionLength * 100.0f)) * abs(luminance.x - luminance.y);

    float3 minimum = min(color00, color11) - colorOffset;
    float3 maximum = max(color00, color11) + colorOffset;

    // Clip history color
    {
        float3 center = 0.5f * (maximum + minimum);
        float3 extents = 0.5f * (maximum - minimum);
        float3 offset = history - center;
        float3 ts = abs(extents / (offset + 0.0001f));
        float t = saturate(min(min(ts.x, ts.y), ts.z));
        history = center + offset * t;
    }

    float weight = clamp(lerp(desc.blendingStatic, desc.blendingMotion, motionLength * desc.motionAmplification), desc.blendingMotion, desc.blendingStatic);

    color = lerp(color, history, weight);
    color = clamp(color, 0.0f, PK_HALF_MAX_MINUS1);

    return color;
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const int2 size = int2(pk_ScreenSize.xy * 2u);

    TAADescriptor desc;
    desc.texelSize = 2.0f.xx / size;
    desc.jitter = pk_ProjectionJitter.xy * desc.texelSize;
    desc.texcoord = (coord + 0.5f.xx) / size;

    const float3 viewpos = SampleViewPosition(desc.texcoord);
    const float2 uv = ViewToPrevClipUV(viewpos);

    desc.motion = (desc.texcoord - uv) + pk_ProjectionJitter.zw * desc.texelSize * 0.5f;
    desc.sharpness = pk_TAA_Sharpness;
    desc.blendingStatic = pk_TAA_BlendingStatic;
    desc.blendingMotion = pk_TAA_BlendingMotion;
    desc.motionAmplification = pk_TAA_MotionAmplification;

    const float3 color = SolveTemporalAntiAliasing(desc);

    imageStore(_HistoryWriteTex, coord, uint4(EncodeE5BGR9(color)));
}