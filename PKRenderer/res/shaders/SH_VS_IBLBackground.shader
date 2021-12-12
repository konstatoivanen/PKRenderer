#version 450
#include includes/Lighting.glsl

#pragma PROGRAM_VERTEX

layout(location = 0) in float4 in_POSITION0;
out float3 vs_TEXCOORD0;

void main()
{
	gl_Position = float4(in_POSITION0.xy, 1.0f, 1.0f);
	float3 vpos = mul(pk_MATRIX_I_P, float4(in_POSITION0.xy, 1.0f, 1.0f)).xyz;
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