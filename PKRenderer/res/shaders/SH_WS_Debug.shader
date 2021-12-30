#version 450
#Cull Back
#ZTest LEqual
#ZWrite True

#MaterialProperty float4 _Color
#MaterialProperty texture2D _AlbedoTexture

#include includes/Common.glsl

#pragma PROGRAM_VERTEX

in float3 in_POSITION;
in float2 in_TEXCOORD0;
out float2 vs_TEXCOORD0;

void main()
{
    gl_Position = ObjectToClipPos(in_POSITION);
    vs_TEXCOORD0 = in_TEXCOORD0;
}

#pragma PROGRAM_FRAGMENT

in float2 vs_TEXCOORD0;
out float4 SV_Target0;

void main()
{
    float4 value = tex2D(_AlbedoTexture, vs_TEXCOORD0);
    SV_Target0 = float4(saturate(value.rgb * _Color.rgb), 1.0);
}