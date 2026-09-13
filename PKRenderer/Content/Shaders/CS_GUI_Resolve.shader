
#pragma pk_program SHADER_STAGE_COMPUTE GUIResolveCs

#include "includes/Common.glsl"

uniform Texture2DMS pk_Texture;
uniform RWTexture2D pk_Image;

[pk_numthreads(8u, 4u, 1u)]
void GUIResolveCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint sampleCount = uint(textureSamples(pk_Texture));

    float4 resolved = 0.0f.xxxx;

    for (uint i = 0u; i < sampleCount; ++i)
    {
        resolved += texelFetch(pk_Texture, coord, int(i));
    }

    resolved /= sampleCount;

    const float4 background = imageLoad(pk_Image, coord);

    float4 final;
    final.rgb = resolved.rgb + background.rgb * (1.0 - resolved.a);
    final.a = resolved.a + background.a * (1.0 - resolved.a);

    imageStore(pk_Image, coord, final);
}
