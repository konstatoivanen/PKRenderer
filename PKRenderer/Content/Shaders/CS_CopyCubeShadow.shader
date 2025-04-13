
#pragma pk_program SHADER_STAGE_COMPUTE main
#include "includes/Utilities.glsl"
#include "includes/Constants.glsl"
#include "includes/Encoding.glsl"

PK_DECLARE_SET_DRAW uniform samplerCubeArray pk_Texture;
PK_DECLARE_SET_DRAW uniform image2DArray pk_Image;

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 size = imageSize(pk_Image).xy;
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = float2(coord + 0.5f.xx) / float2(size);
    const float3 direction = DecodeOctaUv(uv);
    const float4 depth = texture(pk_Texture, float4(direction.x, -direction.y, direction.z, float(gl_GlobalInvocationID.z)));
    imageStore(pk_Image, int3(gl_GlobalInvocationID.xyz), depth);
}