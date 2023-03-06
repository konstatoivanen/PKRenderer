#version 460
#include includes/SharedShadowmapping.glsl
#include includes/SampleDistribution.glsl
#include includes/Blit.glsl
#multi_compile SHADOW_SOURCE_CUBE SHADOW_SOURCE_2D

#define SAMPLE_COUNT 16u
#define SAMPLE_COUNT_INV 0.0625f

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

    #if defined(SHADOW_SOURCE_CUBE)
        float3 N = OctaDecode(uv);
        float3 U = abs(N.z) < 0.999f ? half3(0.0f, 0.0f, 1.0f) : half3(1.0f, 0.0f, 0.0f);
        float3 T = normalize(cross(U, N));
        float3 B = cross(N, T);
        float3 H = float3(0.0f);
        float R = pow5(pk_ShadowmapBlurAmount[vs_LAYER]);

        #pragma unroll SAMPLE_COUNT
        for (uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            float3 offset = GetSampleDirectionHammersLey(PK_HAMMERSLEY_SET_16[i], R);
            H = T * offset.x + B * offset.y + N * offset.z;

            // Cube y axis is flipped to avoid winding order change
            A += tex2D(pk_ShadowmapSource, float4(H.x, -H.y, H.z, layer)).rg;
        }
    
    #elif defined(SHADOW_SOURCE_2D)
        float R = pow2(pk_ShadowmapBlurAmount[vs_LAYER]) * 0.25f;

        #pragma unroll SAMPLE_COUNT
        for (uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            float2 offset = PK_POISSON_DISK_16[i] * R;
            A += tex2D(pk_ShadowmapSource, float3(uv + offset, layer)).rg;
        }
    
    #endif

    SV_Target0 = A * SAMPLE_COUNT_INV;
}