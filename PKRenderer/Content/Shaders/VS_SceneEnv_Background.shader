
#pragma pk_ztest GreaterEqual
#pragma pk_zwrite False
#pragma pk_program Vertex MainVs
#pragma pk_program Fragment MainFs

#include "includes/Common.glsl"
#include "includes/Encoding.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/VolumetricFog.glsl"

struct VsOut
{
    float4 position : SV_Position;
    float3 texcoord : TEXCOORD0;
};

static const float4 PK_BLIT_VERTEX_POSITIONS[3] =
{
    float4(-1.0,  1.0, 0.0, 1.0),
    float4(-1.0, -3.0, 0.0, 1.0),
    float4(3.0,  1.0, 0.0, 1.0),
};

VsOut MainVs(uint vertex_id : SV_VulkanVertexID)
{
    VsOut o;
    o.position = PK_BLIT_VERTEX_POSITIONS[vertex_id];
    o.texcoord = float4(o.position.xy * pk_View.ClipParamsInv.xy, 1.0f, 0.0f) * pk_View.ViewToWorld;
    return o;
}

float4 MainFs(VsOut i) : SV_Target0
{
    const float3 view_dir = normalize(i.texcoord);
    const float2 octa_uv = EncodeOctaUv(view_dir);
    return float4(SceneEnv_Sample_IBL(octa_uv, 0.0f), 1.0f);
}
