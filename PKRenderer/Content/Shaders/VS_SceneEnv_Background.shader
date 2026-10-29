
#pragma pk_ztest GreaterEqual
#pragma pk_zwrite False
#pragma pk_program Vertex MainVs
#pragma pk_program Fragment MainFs

#include "includes/Common.glsl"
#include "includes/Encoding.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/VolumetricFog.glsl"

static const float4 PK_BLIT_VERTEX_POSITIONS[3] =
{
    float4(-1.0f,  1.0f, 0.0f, 1.0f),
    float4(-1.0f, -3.0f, 0.0f, 1.0f),
    float4( 3.0f,  1.0f, 0.0f, 1.0f),
};

struct VsVaryings
{
    float4 sv_Position : SV_Position;
    float3 vs_TEXCOORD0 : TEXCOORD0;
};

VsVaryings MainVs(uint vertexId : SV_VulkanVertexID)
{
    VsVaryings o;
    o.sv_Position = PK_BLIT_VERTEX_POSITIONS[vertexId];
    o.vs_TEXCOORD0 = mul(pk_View.ViewToWorld, float4(o.sv_Position.xy * pk_View.ClipParamsInv.xy, 1.0f, 0.0f));
    return o;
}

float4 MainFs(VsVaryings i) : SV_Target0
{
    const float3 view_dir = normalize(i.vs_TEXCOORD0);
    const float2 octa_uv = EncodeOctaUv(view_dir);
    return float4(SceneEnv_Sample_IBL(octa_uv, 0.0f), 1.0f);
}
