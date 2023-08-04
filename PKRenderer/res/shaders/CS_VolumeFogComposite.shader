#version 460
#pragma PROGRAM_COMPUTE
#include includes/SharedVolumeFog.glsl
#include includes/CTASwizzling.glsl

layout(rgba16f, set = PK_SET_DRAW) uniform image2D _MainTex;

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    int2 coord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    int2 size = imageSize(_MainTex).xy;
    float2 uv = float2(coord + 0.5f.xx) / float2(size);
    float3 color = imageLoad(_MainTex, coord).rgb;
    VolumeFog_Apply(uv, SampleViewDepth(uv), color);
    imageStore(_MainTex, coord, float4(color, 1.0f));
}
