#version 460
#multi_compile PASS_PREFILTER PASS_DISKBLUR

#include includes/SharedDepthOfField.glsl

const uint SAMPLE_COUNT = 22;

uniform sampler2D _MainTex;

#pragma PROGRAM_VERTEX
#include includes/Blit.glsl

#if defined(PASS_PREFILTER)
    out float2 vs_TEXCOORD0;
#else 
    const float2 SAMPLE_KERNEL[SAMPLE_COUNT] =
    {
        float2(0,0),
        float2(0.53333336,0),
        float2(0.3325279,0.4169768),
        float2(-0.11867785,0.5199616),
        float2(-0.48051673,0.2314047),
        float2(-0.48051673,-0.23140468),
        float2(-0.11867763,-0.51996166),
        float2(0.33252785,-0.4169769),
        float2(1,0),
        float2(0.90096885,0.43388376),
        float2(0.6234898,0.7818315),
        float2(0.22252098,0.9749279),
        float2(-0.22252095,0.9749279),
        float2(-0.62349,0.7818314),
        float2(-0.90096885,0.43388382),
        float2(-1,0),
        float2(-0.90096885,-0.43388376),
        float2(-0.6234896,-0.7818316),
        float2(-0.22252055,-0.974928),
        float2(0.2225215,-0.9749278),
        float2(0.6234897,-0.7818316),
        float2(0.90096885,-0.43388376),
    };

    out float3 vs_TEXCOORDS[SAMPLE_COUNT + 1];
#endif

void main()
{
    float2 uv = PK_BLIT_VERTEX_TEXCOORD;
	gl_Position = PK_BLIT_VERTEX_POSITION;

    #if defined(PASS_PREFILTER)
        vs_TEXCOORD0 = uv;
    #else 
    
        float margin = 2.0f / textureSize(_MainTex, 0).y;
    	vs_TEXCOORDS[0] = float3(uv, 1.0f / margin);
    
        for (uint i = 0; i < SAMPLE_COUNT; ++i)
        {
            float2 disp = SAMPLE_KERNEL[i] * pk_MaximumCoC;
            float dist = length(disp);
            disp.x *= pk_ScreenParams.y / pk_ScreenParams.x;
            vs_TEXCOORDS[i + 1] = float3(uv + disp, dist - margin);
        }
    #endif
};

#pragma PROGRAM_FRAGMENT

#if defined(PASS_PREFILTER)
    in float2 vs_TEXCOORD0;
    layout(location = 0) out float4 SV_Target0;
#else 
    in float3 vs_TEXCOORDS[SAMPLE_COUNT + 1];
    layout(location = 0) out float4 SV_Target0;
    layout(location = 1) out float4 SV_Target1;
#endif

void main()
{
    #if defined(PASS_PREFILTER)
        const int2 OFFS[4] = { int2(-1,-1), int2(1,1), int2(-1,1), int2(1,-1) };
        float4 zvalues = textureGatherOffsets(pk_ScreenDepth, vs_TEXCOORD0, OFFS);
        float4 depths = LinearizeDepth(zvalues);

        float4 cocs = GetCirclesOfConfusion(depths);
        float4 weights = saturate(abs(cocs) / pk_MaximumCoC);

        float3 average;
        average.r = dot(textureGatherOffsets(_MainTex, vs_TEXCOORD0, OFFS, 0), weights);
        average.g = dot(textureGatherOffsets(_MainTex, vs_TEXCOORD0, OFFS, 1), weights);
        average.b = dot(textureGatherOffsets(_MainTex, vs_TEXCOORD0, OFFS, 2), weights);
        average /= dot(weights, 1.0f.xxxx);

        SV_Target0 = float4(average, dot(cocs, 0.25f.xxxx));
    #else
        float4 center = tex2D(_MainTex, vs_TEXCOORDS[0].xy);
        float4 background = float4(0.0f); 
        float4 foreground = float4(0.0f); 

        #pragma unroll SAMPLE_COUNT
        for (uint i = 1; i <= SAMPLE_COUNT; ++i)
        {
            float4 value = tex2D(_MainTex, vs_TEXCOORDS[i].xy);
            float backgroundCoC = max(min(center.a, value.a), 0);
            background += float4(value.rgb, 1.0f) * saturate((backgroundCoC - vs_TEXCOORDS[i].z) * vs_TEXCOORDS[0].z);
            foreground += float4(value.rgb, 1.0f) * saturate((-value.a - vs_TEXCOORDS[i].z) * vs_TEXCOORDS[0].z);
        }

        background.rgb /= background.a + (background.a < 1e-4f ? 1.0f : 0.0f);
        foreground.rgb /= foreground.a + (foreground.a < 1e-4f ? 1.0f : 0.0f);
        
        //background.a = smoothstep(_MainTex_TexelSize.y, _MainTex_TexelSize.y * 2, center.a);
        foreground.a = saturate(foreground.a * PK_PI / SAMPLE_COUNT);

        SV_Target0 = foreground;
        SV_Target1 = background;
    #endif
};

