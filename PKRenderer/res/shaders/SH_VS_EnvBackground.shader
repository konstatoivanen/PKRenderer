#version 450
#ZTest GEqual
#ZWrite False
#include includes/Common.glsl
#include includes/Encoding.glsl
#include includes/SceneEnv.glsl
#include includes/VolumeFog.glsl

#pragma PROGRAM_VERTEX

float4 PK_BLIT_VERTEX_POSITIONS[3] =
{
    float4(-1.0,  1.0, 0.0, 1.0),
    float4(-1.0, -3.0, 0.0, 1.0),
    float4( 3.0,  1.0, 0.0, 1.0),
};

out float3 vs_TEXCOORD0;

void main()
{
    gl_Position = PK_BLIT_VERTEX_POSITIONS[gl_VertexIndex];
    vs_TEXCOORD0 = float4(gl_Position.xy * pk_ClipParamsInv.xy, 1.0f, 0.0f) * pk_ViewToWorld;
}

#pragma PROGRAM_FRAGMENT

in float3 vs_TEXCOORD0;
layout(location = 0) out float4 SV_Target0;

void main()
{
    const float3 viewdir = normalize(vs_TEXCOORD0);
    const float2 octaUV = OctaUV(viewdir);
    float3 color = SampleEnvironment(octaUV, 0.0f);
    VFog_ApplySky(viewdir, color);
    SV_Target0 = float4(color, 1.0f);
}