#pragma pk_program SHADER_STAGE_COMPUTE main
#include "includes/Common.glsl"
#include "includes/Noise.glsl"

PK_DECLARE_SET_DRAW uniform image2D pk_Image;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    const float phase = make_unorm(pk_FrameRandom.x);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float3 grain = NoiseGrainColor(coord, phase, 256.0f);
    imageStore(pk_Image, coord, float4(grain, 1.0f));
}