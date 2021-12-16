#version 450
#pragma PROGRAM_COMPUTE
#include includes/HLSLSupport.glsl

PK_DECLARE_SET_DRAW uniform sampler2D _MainTex;
layout(rgba16f, set = PK_SET_DRAW) uniform image2D _OutImage;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(_OutImage).xy;

    if (Greater(coord, size))
    {
        return;
    }

    float2 uv = float2(coord) / float2(size);
    float4 value = float4(saturate(1.0f - tex2D(_MainTex, uv).rgb), 1.0f);
    
    imageStore(_OutImage, coord, value);
}