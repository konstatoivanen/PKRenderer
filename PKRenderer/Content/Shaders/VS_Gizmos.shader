#PK_BlendColor Add SrcAlpha OneMinusSrcAlpha
#PK_ZTest Off
#PK_ZWrite False
#PK_Cull Off

#include "includes/Common.glsl"
#include "includes/GBuffers.glsl"

#pragma PROGRAM_VERTEX

in uint4 in_POSITION;
out float4 vs_COLOR;

void main()
{
    gl_Position = pk_WorldToClip_NoJitter * float4(uintBitsToFloat(in_POSITION.xyz), 1.0f);
    vs_COLOR = unpackUnorm4x8(in_POSITION.w);
};

#pragma PROGRAM_FRAGMENT

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
