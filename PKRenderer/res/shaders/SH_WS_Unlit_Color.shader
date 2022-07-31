#version 450
#Cull Back
#ZTest LEqual
#ZWrite Off

#MaterialProperty float4 _Color

#include includes/Common.glsl

#pragma PROGRAM_VERTEX
in float3 in_POSITION;

void main()
{
    gl_Position = ObjectToClipPos(float4(in_POSITION, 0));
    NORMALIZE_GL_Z;
}

#pragma PROGRAM_FRAGMENT
out float4 SV_Target0;

void main()
{
    SV_Target0 = _Color;
}