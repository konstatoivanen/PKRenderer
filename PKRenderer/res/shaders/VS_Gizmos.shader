#BlendColor Add SrcAlpha OneMinusSrcAlpha
#ZTest Off
#ZWrite False
#Cull Off

#include includes/Common.glsl

#pragma PROGRAM_VERTEX

in uint4 in_POSITION;
out float4 vs_COLOR;

void main()
{
    float4 color = 0.0f.xxxx;
    color.r = ((in_POSITION.w >> 0) & 0xFFu) / 255.0f;
    color.g = ((in_POSITION.w >> 8u) & 0xFFu) / 255.0f;
    color.b = ((in_POSITION.w >> 16u) & 0xFFu) / 255.0f;
    color.a = ((in_POSITION.w >> 24u) & 0xFFu) / 255.0f;
    
    gl_Position = pk_WorldToClip_NoJitter * float4(uintBitsToFloat(in_POSITION.xyz), 1.0f);
    vs_COLOR = color;
};

#pragma PROGRAM_FRAGMENT
#include includes/GBuffers.glsl

in float4 vs_COLOR;
layout(location = 0) out float4 SV_Target0;

void main()
{
    const float viewDepth = ViewDepth(gl_FragCoord.z);
    const float sceneDepth = SampleViewDepth(gl_FragCoord.xy * pk_ScreenParams.zw);
    const float alphaFade = 0.3f * exp((-viewDepth + sceneDepth) * 0.125f);

    float4 color = vs_COLOR;
    color.a *= sceneDepth < viewDepth ? alphaFade : 1.0f;

    SV_Target0 = color;
};