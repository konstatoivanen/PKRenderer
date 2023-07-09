#version 460
#BlendColor Add One SrcAlpha
#include includes/SharedDepthOfField.glsl

#pragma PROGRAM_VERTEX
#include includes/Blit.glsl

out float2 vs_TEXCOORD0;

void main()
{
    gl_Position = PK_BLIT_VERTEX_POSITION;
    vs_TEXCOORD0 = PK_BLIT_VERTEX_TEXCOORD;
};

#pragma PROGRAM_FRAGMENT

PK_DECLARE_SET_PASS uniform sampler2D pk_Foreground;
PK_DECLARE_SET_PASS uniform sampler2D pk_Background;

in float2 vs_TEXCOORD0;
layout(location = 0) out float4 SV_Target0;

void main()
{
    float viewDepth = SampleViewDepth(vs_TEXCOORD0);
    float coc = GetCircleOfConfusion(viewDepth);

    float4 foreground = tex2D(pk_Foreground, vs_TEXCOORD0);
    float4 background = tex2D(pk_Background, vs_TEXCOORD0);

    float texely = 1.0f / textureSize(pk_Foreground, 0).y;
    background.a = smoothstep(texely, texely * 2.0f, coc);

    float3 color = lerp(background.rgb * background.a, foreground.rgb, foreground.a);

    SV_Target0 = float4(color, (1.0f - foreground.a) * (1.0f - background.a));
};