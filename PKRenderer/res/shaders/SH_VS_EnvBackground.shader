#version 450
#ZTest LEqual
#ZWrite False
#include includes/Common.glsl
#include includes/Encoding.glsl
#include includes/SceneEnv.glsl
#include includes/Blit.glsl
#include includes/SharedVolumeFog.glsl

#pragma PROGRAM_VERTEX

out float3 vs_TEXCOORD0;

void main()
{
    gl_Position = PK_BLIT_VERTEX_POSITION;
    vs_TEXCOORD0 = mul(float4(gl_Position.xy * pk_InvProjectionParams.xy, 1.0f, 0.0f), pk_MATRIX_I_V).xyz;
}

#pragma PROGRAM_FRAGMENT

in float3 vs_TEXCOORD0;
layout(location = 0) out float4 SV_Target0;

void main()
{
    const float3 viewdir = normalize(vs_TEXCOORD0);
    const float2 octaUV = OctaUV(viewdir);
    float3 color = SampleEnvironment(octaUV, 0.0f);
    VolumeFog_ApplySky(viewdir, color);
    SV_Target0 = float4(color, 1.0f);
}