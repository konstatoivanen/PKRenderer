#pragma pk_program SHADER_STAGE_COMPUTE main
#include "includes/Common.glsl"
#include "includes/Noise.glsl"

layout(rgba8, set = PK_SET_DRAW) uniform image2D pk_Image;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    const float modTime = mod(pk_Time.x, 10.0f);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const int2 size = imageSize(pk_Image).xy;

    float4 value = float4(NoiseGrainColor(float2(coord + 0.5f.xx) / float2(size), modTime) * 10.0f, 1.0f);
    imageStore(pk_Image, coord, value);
}