
#pragma pk_program Compute CopyCubeShadowCs
#include "includes/Common.glsl"
#include "includes/Utilities.glsl"
#include "includes/Constants.glsl"
#include "includes/Encoding.glsl"

uniform TextureCubeArrayRef pk_Texture;
uniform RWTexture2DArrayRef<float4> pk_Image;

[numthreads(8u, 8u, 1u)]
void CopyCubeShadowCs()
{
    const int2 size = int2(pk_Image.GetSize().xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = float2(coord + 0.5f.xx) / float2(size);
    const float3 direction = DecodeOctaUv(uv);
    const float4 depth = pk_Texture.SampleLevel(pk_SamplerBilinearMirror, float4(direction.x, -direction.y, direction.z, float(gl_GlobalInvocationID.z)), 0.0f);
    pk_Image[int3(gl_GlobalInvocationID.xyz)] = depth;
}