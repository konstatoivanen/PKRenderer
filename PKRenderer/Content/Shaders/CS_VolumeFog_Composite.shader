
#pragma pk_program SHADER_STAGE_COMPUTE main

#include "includes/VolumeFog.glsl"
#include "includes/CTASwizzling.glsl"

PK_DECLARE_SET_DRAW uniform image2D pk_Image;

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    const int2 size = imageSize(pk_Image).xy;
    const float2 uv = float2(coord + 0.5f.xx) / float2(size);
    
    const float3 color = imageLoad(pk_Image, coord).rgb;
    const float4 color_transmittance = Fog_SampleFroxel(uv, SampleViewDepth(uv), color);

    imageStore(pk_Image, coord, color_transmittance);
}