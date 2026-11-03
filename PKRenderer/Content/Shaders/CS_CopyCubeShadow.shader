
#pragma pk_program Compute CopyCubeShadowCs

#include "includes/Common.glsl"
#include "includes/Encoding.glsl"

uniform TextureCubeArray<float> pk_Texture;
uniform RWTexture2DArray<float> pk_Image;

[numthreads(8u, 8u, 1u)]
void CopyCubeShadowCs(int3 coord : SV_DispatchThreadID)
{
    const int2 size = pk_Image.GetSize().xy;
    const float2 uv = float2(coord.xy + 0.5f.xx) / float2(size);
    const float3 direction = DecodeOctaUv(uv);
    const float depth = pk_Texture.SampleLevel(pk_SamplerBilinearMirror, float4(direction.x, -direction.y, direction.z, float(coord.z)), 0);
    pk_Image[coord] = depth;
}
