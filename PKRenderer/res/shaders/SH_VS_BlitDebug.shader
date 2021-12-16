#version 450
#include includes/HLSLSupport.glsl

#pragma PROGRAM_VERTEX
#include includes/Blit.glsl

out float2 vs_TEXCOORD0;
void main()
{
	gl_Position = PK_BLIT_VERTEX_POSITION;
	vs_TEXCOORD0 = PK_BLIT_VERTEX_TEXCOORD;
}

#pragma PROGRAM_FRAGMENT

layout(rgba16f, set = PK_SET_DRAW) uniform image2D _MainTex;

in float2 vs_TEXCOORD0;
layout(location = 0) out float4 SV_Target0;

void main()
{
	int2 pxcoord = int2(vs_TEXCOORD0 * imageSize(_MainTex).xy);
	float4 value = imageLoad(_MainTex, pxcoord);
	SV_Target0 = float4(saturate(1.0f - value.rgb), 1.0f);
}