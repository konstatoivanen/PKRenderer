
#pragma pk_program Compute GUIResolveCs

#include "includes/Common.glsl"

uniform Texture2DMS<float4> pk_Texture;
uniform RWTexture2D<float4> pk_Image;

[numthreads(8u, 4u, 1u)]
void GUIResolveCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint sampleCount = pk_Texture.GetSampleCount();

    float4 resolved = 0.0f.xxxx;

    for (uint i = 0u; i < sampleCount; ++i)
    {
        resolved += pk_Texture.Load(int3(coord, i));
    }

    resolved /= sampleCount;

    const float4 background = pk_Image[coord];

    float4 final;
    final.rgb = resolved.rgb + background.rgb * (1.0 - resolved.a);
    final.a = resolved.a + background.a * (1.0 - resolved.a);

    pk_Image[coord] = final;
}
