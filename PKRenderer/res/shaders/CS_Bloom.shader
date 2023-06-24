#version 450
#multi_compile PASS_DOWNSAMPLE PASS_BLUR

#pragma PROGRAM_COMPUTE
#include includes/Utilities.glsl
#include includes/Encoding.glsl

#if defined(PASS_BLUR)
PK_DECLARE_LOCAL_CBUFFER(_BlurOffset)
{
    float2 blurOffset;
};
#endif

const float sample_weights[17] =
{
    0.0006428483,
    0.002363721,
    0.007306095,
    0.0189835,
    0.04146377,
    0.07613125,
    0.1175057,
    0.1524602,
    0.1662859,
    0.1524602,
    0.1175057,
    0.07613125,
    0.04146377,
    0.0189835,
    0.007306095,
    0.002363721,
    0.0006428483,
};

PK_DECLARE_SET_DRAW uniform sampler2D _SourceTex;
layout(r32ui, set = PK_SET_DRAW) uniform uimage2D _DestinationTex;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(_DestinationTex).xy;

    if (Any_GEqual(coord, size))
    {
        return;
    }

    float2 uv = float2(coord + 0.5f.xx) / float2(size);
    float2 texel = 1.0f.xx / textureSize(_SourceTex, 0).xy;
    float3 color = 0.0f.xxx;

#if defined(PASS_DOWNSAMPLE)
    color += tex2D(_SourceTex, uv + 0.5f * texel).rgb;
    color += tex2D(_SourceTex, uv - 0.5f * texel).rgb;
    color += tex2D(_SourceTex, uv + float2(0.5f, -0.5f) * texel).rgb;
    color += tex2D(_SourceTex, uv - float2(0.5f, -0.5f) * texel).rgb;
    color = max(color / 4.0f, 0.0f.xxx);
#else
    float2 offs = blurOffset * texel;
    float2 coords = uv - offs * 8.0f;

    for (uint i = 0u; i < 17; ++i)
    {
        color += tex2D(_SourceTex, coords + offs * i).rgb * sample_weights[i].xxx;
    }

#endif

    imageStore(_DestinationTex, coord, uint4(EncodeE5BGR9(color)));
}