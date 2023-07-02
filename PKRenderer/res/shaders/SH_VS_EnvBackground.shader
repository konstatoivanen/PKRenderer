#version 450
#ZTest LEqual
#ZWrite False
#include includes/Common.glsl
#include includes/Encoding.glsl
#include includes/SceneEnv.glsl
#include includes/Blit.glsl

#pragma PROGRAM_VERTEX

out float3 vs_TEXCOORD0;

void main()
{
    gl_Position = PK_BLIT_VERTEX_POSITION;
    float3 vpos = mul(pk_MATRIX_I_P, float4(gl_Position.xy, 1.0f, 1.0f)).xyz;
    vs_TEXCOORD0 = mul(pk_MATRIX_I_V, float4(vpos, 0.0f)).xyz;
}

#pragma PROGRAM_FRAGMENT

in float3 vs_TEXCOORD0;
layout(location = 0) out float4 SV_Target0;

void main()
{
    float2 reflUV = OctaUV(normalize(vs_TEXCOORD0));
    float3 color = SampleEnvironment(reflUV, 0.0f);
    SV_Target0 = float4(color, 1.0f);
}