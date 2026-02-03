
#pragma pk_ztest GreaterEqual
#pragma pk_zwrite False
#pragma pk_program SHADER_STAGE_VERTEX MainVs
#pragma pk_program SHADER_STAGE_FRAGMENT MainFs

#include "includes/Common.glsl"
#include "includes/Encoding.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/VolumeFog.glsl"

PK_DECLARE_VS_ATTRIB(float3 vs_TEXCOORD0);

[pk_local(MainFs)] out float4 SV_Target0;

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
    const float3 view_dir = normalize(vs_TEXCOORD0);
    const float2 octa_uv = EncodeOctaUv(view_dir);
    SV_Target0 = float4(SceneEnv_Sample_IBL(octa_uv, 0.0f), 1.0f);
}
