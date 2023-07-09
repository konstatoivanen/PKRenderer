#version 460
#include includes/Utilities.glsl
#include includes/Constants.glsl
#include includes/Kernels.glsl
#include includes/Encoding.glsl
#include includes/SampleDistribution.glsl
#include includes/Blit.glsl

#multi_compile SHADOW_SOURCE_CUBE SHADOW_SOURCE_2D

#pragma PROGRAM_VERTEX
out float2 vs_TEXCOORD0;
out flat uint vs_LAYER;

void main()
{
    gl_Position = PK_BLIT_VERTEX_POSITION;
    gl_Layer = gl_InstanceIndex;
    vs_LAYER = gl_InstanceIndex;
    vs_TEXCOORD0 = PK_BLIT_VERTEX_TEXCOORD;
}

#pragma PROGRAM_FRAGMENT

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

in float2 vs_TEXCOORD0;
in flat uint vs_LAYER;
out float2 SV_Target0;

void main()
{
    float2 uv = vs_TEXCOORD0;
    float layer = vs_LAYER;
    float2 A = float2(0.0f);
    float R = pk_ShadowmapBlurAmount[vs_LAYER];

#if defined(SHADOW_SOURCE_CUBE)
    float3 N = OctaDecode(uv);
    float3x3 TBN = ComposeTBNFast(N);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float3 H = TBN * ConeDirectionHammersLey(i, SAMPLE_COUNT, R);
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

    SV_Target0 = A * SAMPLE_COUNT_INV;
}