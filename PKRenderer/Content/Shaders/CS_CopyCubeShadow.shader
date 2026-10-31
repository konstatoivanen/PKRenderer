
#pragma pk_program Compute CopyCubeShadowCs

#include "includes/Common.glsl"
#include "includes/Encoding.glsl"

uniform TextureCubeArray<float> pk_Texture;
uniform RWTexture2DArray<float> pk_Image;

[numthreads(8u, 8u, 1u)]
void CopyCubeShadowCs()
{
    const int2 size = imageSize(pk_Image).xy;
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = float2(coord + 0.5f.xx) / float2(size);
    const float3 direction = DecodeOctaUv(uv);
    const float4 depth = TMP_SMP_CubeArray(pk_Texture, pk_SamplerBilinearMirror, float4(direction.x, -direction.y, direction.z, float(gl_GlobalInvocationID.z)));
    imageStore(pk_Image, int3(gl_GlobalInvocationID.xyz), depth);
}