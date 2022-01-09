#version 460
#include includes/SharedShadowmapping.glsl
#include includes/Blit.glsl
#multi_compile SHADOW_SOURCE_CUBE SHADOW_SOURCE_2D
#multi_compile SHADOW_BLUR_PASS0 SHADOW_BLUR_PASS1

#define SAMPLE_COUNT 5u
#define SAMPLE_COUNT_INV 0.2f

#pragma PROGRAM_VERTEX

const float3 SAMPLES_HAMMERSLEY_3D[SAMPLE_COUNT] =
{
    float3(1.0f,       0.0f,       0.0f),
    float3(0.309017f,  0.9510565f, 0.5f),
    float3(-0.8090171f, 0.5877852f, 0.25f),
    float3(-0.8090169f,-0.5877854f, 0.75f),
    float3(0.3090171f,-0.9510565f, 0.125f),
};

float3 DistributeHammersley3D(float3 Xi, float blur)
{
    float theta = (1.0f - Xi.z) / (1.0f + (blur - 1.0f) * Xi.z);
    float2 sincos = sqrt(float2(1.0f - theta, theta));
    return normalize(float3(Xi.xy * sincos.xx, sincos.y));
}

out float3 vs_TEXCOORD0;
#if defined(SHADOW_SOURCE_CUBE)
    out flat float3[SAMPLE_COUNT] vs_OFFSETS;
#endif

void main()
{
    gl_Position = PK_BLIT_VERTEX_POSITION;
    gl_Layer = gl_InstanceIndex;
    vs_TEXCOORD0 = float3(PK_BLIT_VERTEX_TEXCOORD, gl_InstanceIndex);

    #if defined(SHADOW_SOURCE_CUBE)
        float R = pow5(pk_ShadowmapBlurAmount[gl_Layer]);

        #pragma unroll SAMPLE_COUNT
        for (uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            vs_OFFSETS[i] = DistributeHammersley3D(SAMPLES_HAMMERSLEY_3D[i], R);
        }
    #endif
}

#pragma PROGRAM_FRAGMENT

const int2 sample_offsets[4] = { int2(1,0), int2(0,1), int2(-1,0), int2(0,-1) };
const int2 sample_offsets_h0[4] = { int2(-2, 0), int2(-1,0), int2(1,0), int2(2,0) };
const int2 sample_offsets_h1[4] = { int2(-3, 0), int2(0, 0), int2(3,0), int2(0,0) };

const int2 sample_offsets_v0[4] = { int2(0,-2), int2(0,-1), int2(0,1), int2(0,2) };
const int2 sample_offsets_v1[4] = { int2(0,-3), int2(0, 0), int2(0,3), int2(0,0) };

#if defined(SHADOW_BLUR_PASS0)
    #if defined(SHADOW_SOURCE_CUBE)
        PK_DECLARE_SET_DRAW uniform samplerCubeArray pk_ShadowmapSource;
        // Cube y axis is flipped to avoid winding order change
        float2 SAMPLE_SRC(float3 H, float layer) { return tex2D(pk_ShadowmapSource, float4(H.x, -H.y, H.z, layer)).rg; }
    #elif defined(SHADOW_SOURCE_2D)
        PK_DECLARE_SET_DRAW uniform sampler2DArray pk_ShadowmapSource;
        float2 SAMPLE_SRC(float3 uvw)
        {
            float4 valueR0 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_h0, 0);
            float4 valueG0 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_h0, 1);
            float3 valueR1 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_h1, 0).xyz;
            float3 valueG1 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_h1, 1).xyz;

            return float2
            (
                dot(valueR0, 0.1428571428571429.xxxx) + dot(valueR1, 0.1428571428571429.xxx),
                dot(valueG0, 0.1428571428571429.xxxx) + dot(valueG1, 0.1428571428571429.xxx)
            );
        }
    #endif
#else

    PK_DECLARE_SET_DRAW uniform sampler2DArray pk_ShadowmapSource;

    #if defined(SHADOW_SOURCE_CUBE)
        float2 SAMPLE_SRC(float3 H, float layer) 
        {
            float3 uvw = float3(OctaUV(H), layer);
            return float2(dot(textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets, 0), 0.25f.xxxx), 
                          dot(textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets, 1), 0.25f.xxxx));
        }
    #elif defined(SHADOW_SOURCE_2D)
        float2 SAMPLE_SRC(float3 uvw)
        {
            float4 valueR0 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_v0, 0);
            float4 valueG0 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_v0, 1);
            float3 valueR1 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_v1, 0).xyz;
            float3 valueG1 = textureGatherOffsets(pk_ShadowmapSource, uvw, sample_offsets_v1, 1).xyz;

            return float2
            (
                dot(valueR0, 0.1428571428571429.xxxx) + dot(valueR1, 0.1428571428571429.xxx),
                dot(valueG0, 0.1428571428571429.xxxx) + dot(valueG1, 0.1428571428571429.xxx)
            );
        }
    #endif
#endif

in float3 vs_TEXCOORD0;
#if defined(SHADOW_SOURCE_CUBE)
    in flat float3[SAMPLE_COUNT] vs_OFFSETS;
#endif
out float2 SV_Target0;

void main()
{
    #if defined(SHADOW_SOURCE_CUBE)
        float2 uv = vs_TEXCOORD0.xy;
        float3 N = OctaDecode(uv);
        float3 U = abs(N.z) < 0.999f ? half3(0.0f, 0.0f, 1.0f) : half3(1.0f, 0.0f, 0.0f);
        float3 T = normalize(cross(U, N));
        float3 B = cross(N, T);
        float3 H = float3(0.0f);
        float2 A = float2(0.0f);
    
        #pragma unroll SAMPLE_COUNT
        for (uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            H = T * vs_OFFSETS[i].x + B * vs_OFFSETS[i].y + N * vs_OFFSETS[i].z;
            A += SAMPLE_SRC(H, vs_TEXCOORD0.z);
        }
    
        SV_Target0 = A * SAMPLE_COUNT_INV;
    #elif defined(SHADOW_SOURCE_2D)
        SV_Target0 = SAMPLE_SRC(vs_TEXCOORD0);
    #else
        SV_Target0 = 0.0f.xx;
    #endif
}