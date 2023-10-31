#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/PostFXResources.glsl
#include includes/Encoding.glsl

PK_DECLARE_SET_DRAW uniform sampler2D pk_Texture; // Current Screen
PK_DECLARE_SET_DRAW uniform sampler2D pk_Texture1; // History Read
layout(r32ui, set = PK_SET_DRAW) uniform uimage2D pk_Image; // History Write

#define SAMPLE_TAA_SOURCE(uv) texture(pk_Texture, uv)
#define SAMPLE_TAA_HISTORY(uv) texture(pk_Texture1, uv)

struct TAADescriptor
{
    float2 texelSize;
    float2 jitter;
    float2 texcoord;
    float2 motion;
    float alphaMult;
    float sharpness;
    float blendStatic;
    float blendMotion;
    float motionMult;
};

float3 SolveTemporalAntiAliasing(TAADescriptor desc)
{
    float2 uv = desc.texcoord - desc.jitter;

    float4 colorAlpha = SAMPLE_TAA_SOURCE(uv);
    float3 color = colorAlpha.rgb;
    float3 history = SAMPLE_TAA_HISTORY(desc.texcoord - desc.motion).rgb;
    float3 color11 = SAMPLE_TAA_SOURCE(uv - desc.texelSize * 0.5f).rgb;
    float3 color00 = SAMPLE_TAA_SOURCE(uv + desc.texelSize * 0.5f).rgb;
    float3 corners = 4.0f * (color11 + color00) - 2.0f * color;

    color += (color - (corners * 0.166667f)) * 2.718282f * desc.sharpness;
    color = clamp(color, 0.0, PK_HALF_MAX_MINUS1);

    float3 average = (corners + color) * 0.142857f;
    float2 luminance = float2(dot(average, PK_LUMA_BT709), dot(color, PK_LUMA_BT709));

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

    float weight = lerp(desc.blendStatic, desc.blendMotion, motionLength * desc.motionMult);
    weight *= lerp(1.0f, colorAlpha.a, desc.alphaMult);
    weight = clamp(weight, desc.blendMotion, desc.blendStatic);

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

    const float depth = SampleViewDepth(desc.texcoord);
    const float3 viewpos = UVToViewPos(desc.texcoord, depth);
    const float2 uv = ViewToClipUVPrev(viewpos);

    desc.motion = (desc.texcoord - uv) + pk_ProjectionJitter.zw * desc.texelSize * 0.5f;
    desc.sharpness = pk_TAA_Sharpness;
    desc.blendStatic = pk_TAA_BlendingStatic;
    desc.blendMotion = pk_TAA_BlendingMotion;
    desc.motionMult = pk_TAA_MotionAmplification;
    desc.alphaMult = 0.0f;

    const float3 color = SolveTemporalAntiAliasing(desc);

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(color)));
}