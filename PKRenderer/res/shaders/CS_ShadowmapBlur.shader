#version 460
#pragma PROGRAM_COMPUTE
#include includes/Utilities.glsl
#include includes/Constants.glsl
#include includes/Kernels.glsl
#include includes/Encoding.glsl
#include includes/Noise.glsl
#include includes/CTASwizzling.glsl

#multi_compile SHADOW_SOURCE_CUBE SHADOW_SOURCE_2D
#WithAtomicCounter

#define SAMPLE_COUNT 16u
#define SAMPLE_COUNT_INV 0.0625f

PK_DECLARE_LOCAL_CBUFFER(pk_ShadowmapData)
{
    float4 pk_ShadowmapBlurAmount;
};

#if defined(SHADOW_SOURCE_CUBE)
PK_DECLARE_SET_DRAW uniform samplerCubeArray pk_ShadowmapSource;
#elif defined(SHADOW_SOURCE_2D)
PK_DECLARE_SET_DRAW uniform sampler2DArray pk_ShadowmapSource;
#endif

layout(rg32f, set = PK_SET_DRAW) uniform image2DArray pk_Image;

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const uint thread = PK_AtomicCounterNext();
    const int2 size = imageSize(pk_Image).xy;
    const uint3 threadID = GetZCurveSwizzle32(thread, uint2(size));
    const int2 coord = int2(threadID.xy);
    const float2 uv = float2(coord + 0.5f.xx) / float2(size);
    const float layer = float(threadID.z);
    const float R = pk_ShadowmapBlurAmount[gl_GlobalInvocationID.z];

    float2 A = float2(0.0f);

#if defined(SHADOW_SOURCE_CUBE)
    const float3 N = OctaDecode(uv);
    const float3x3 TBN = make_TBN(N);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        const float2 Xi = Hammersley(i, SAMPLE_COUNT);
        const float theta = Xi.x * R;
        const float phi = PK_TWO_PI * Xi.y;
        const float3 H = TBN * float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
        
        // Cube y axis is flipped to avoid winding order change
        A += tex2D(pk_ShadowmapSource, float4(H.x, -H.y, H.z, layer)).rg;
    }

#elif defined(SHADOW_SOURCE_2D)

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 offset = PK_POISSON_DISK_16[i] * R * 0.5f;
        A += tex2D(pk_ShadowmapSource, float3(uv + offset, layer)).rg;
    }

#endif

    imageStore(pk_Image, int3(threadID), float4(A * SAMPLE_COUNT_INV, 0.0f.xx));
}