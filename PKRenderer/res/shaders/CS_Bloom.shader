#version 450
#multi_compile PASS_DOWNSAMPLE PASS_BLUR

#pragma PROGRAM_COMPUTE
#include includes/Utilities.glsl

#if defined(PASS_BLUR)
    PK_DECLARE_LOCAL_CBUFFER(_BlurOffset)
    {
        float2 blurOffset;
    };
#endif

PK_DECLARE_SET_DRAW uniform sampler2D _SourceTex;
layout(rgba16f, set = PK_SET_DRAW) uniform image2D _DestinationTex;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(_DestinationTex).xy;

    if (GEqual(coord, size))
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
        float2 coords = uv - offs * 3.0f;

        color += tex2D(_SourceTex, coords + offs * 0.0f).rgb * 0.0205f.xxx;
        color += tex2D(_SourceTex, coords + offs * 1.0f).rgb * 0.0855f.xxx;
        color += tex2D(_SourceTex, coords + offs * 2.0f).rgb * 0.232f.xxx;
        color += tex2D(_SourceTex, coords + offs * 3.0f).rgb * 0.324f.xxx;
        color += tex2D(_SourceTex, coords + offs * 4.0f).rgb * 0.232f.xxx;
        color += tex2D(_SourceTex, coords + offs * 5.0f).rgb * 0.0855f.xxx;
        color += tex2D(_SourceTex, coords + offs * 6.0f).rgb * 0.0205f.xxx;
    #endif

    imageStore(_DestinationTex, coord, float4(color, 1.0f));
}