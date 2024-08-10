
#pragma pk_ztest GEqual
#pragma pk_zwrite False
#pragma pk_program SHADER_STAGE_VERTEX MainVs
#pragma pk_program SHADER_STAGE_FRAGMENT MainFs
#define PK_USE_SINGLE_DESCRIPTOR_SET

#include "includes/Common.glsl"
#include "includes/Encoding.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/VolumeFog.glsl"

PK_DECLARE_VS_ATTRIB(float3 vs_TEXCOORD0);

[[pk_restrict MainFs]] out float4 SV_Target0;

float4 PK_BLIT_VERTEX_POSITIONS[3] =
{
    float4(-1.0,  1.0, 0.0, 1.0),
    float4(-1.0, -3.0, 0.0, 1.0),
    float4(3.0,  1.0, 0.0, 1.0),
};

void MainVs()
{
    gl_Position = PK_BLIT_VERTEX_POSITIONS[gl_VertexIndex];
    vs_TEXCOORD0 = float4(gl_Position.xy * pk_ClipParamsInv.xy, 1.0f, 0.0f) * pk_ViewToWorld;
}

void MainFs()
{
    const float3 viewdir = normalize(vs_TEXCOORD0);
    const float2 octaUV = OctaUV(viewdir);
    SV_Target0 = float4(SampleEnvironment(octaUV, 0.0f), 1.0f);
}
