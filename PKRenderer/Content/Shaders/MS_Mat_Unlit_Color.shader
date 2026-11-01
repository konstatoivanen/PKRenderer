
#pragma pk_cull Back
#pragma pk_ztest GreaterEqual
#pragma pk_zwrite Off

#pragma pk_material_property float4 _Color
#pragma pk_material_property float4 _ColorVoxelize

#pragma pk_multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#pragma pk_program Fragment ForwardFs
#pragma pk_program Fragment GBufferFs PK_META_PASS_GBUFFER
#pragma pk_program Fragment GIVoxelizeFs PK_META_PASS_GIVOXELIZE

#define PK_MESHLET_USE_FUNC_CULL 1

#if defined(PK_META_PASS_GBUFFER)
struct UnlitVertexUserData
{
    float3 vs_Normal : NORMAL0;
};
#define PK_MESHLET_VERTEX_USER_DATA UnlitVertexUserData
#endif

#include "includes/GBuffers.glsl"
#include "includes/Meshlets.glsl"
#include "includes/SceneGIVX.glsl"

bool PK_MESHLET_FUNC_CULL(PKMeshTaskPayload payload, const PKMeshlet meshlet)
{
    return Meshlet_Cull_Cone(meshlet, pk_View.ViewWorldOrigin.xyz);
}

void PK_MESHLET_FUNC_VERTEX(uint vertex_index, PKMeshTaskPayload payload, PKVertex vertex, inout PKMeshletVertexVaryings outVaryings)
{
    outVaryings.sv_Position = ObjectToClipPos(vertex.position);
#if defined(PK_META_PASS_GBUFFER)
    outVaryings.userdata.vs_Normal = ObjectToWorldVec(vertex.normal);
#endif
}

float4 ForwardFs(PKMeshletVertexVaryings vout) : SV_Target0
{
    PK_INSTANCING_ASSIGN_FRAGMENT_LOCALS(vout);
    return _Color;
}

#if defined(PK_META_PASS_GBUFFER)
float4 GBufferFs(PKMeshletVertexVaryings vout) : SV_Target0
{
    PK_INSTANCING_ASSIGN_FRAGMENT_LOCALS(vout);
    return EncodeGBufferWorldNR(vout.userdata.vs_Normal, 0.0f, 0.0f);
}
#endif

void GIVoxelizeFs(PKMeshletVertexVaryings vout)
{
    PK_INSTANCING_ASSIGN_FRAGMENT_LOCALS(vout);
    GI_VX_Store(GI_VX_FragToWorld(vout.sv_Position.xyz), _ColorVoxelize);
}
