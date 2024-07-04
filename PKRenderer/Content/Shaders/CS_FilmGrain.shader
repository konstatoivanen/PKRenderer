#pragma PROGRAM_COMPUTE
#include "includes/Common.glsl"
#include "includes/Noise.glsl"

layout(rgba8, set = PK_SET_DRAW) uniform image2D pk_Image;

shared float pk_ModTime;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    if (gl_LocalInvocationIndex == 0)
    {
        pk_ModTime = mod(pk_Time.x, 10.0f);
    }

    barrier();

    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(pk_Image).xy;
    float4 value = float4(NoiseGrainColor(float2(coord + 0.5f.xx) / float2(size), pk_ModTime) * 10.0f, 1.0f);
    imageStore(pk_Image, coord, value);
}