
#pragma pk_cull Back
#pragma pk_ztest GreaterEqual
#pragma pk_zwrite Off

#pragma pk_material_property float4 _Color
#pragma pk_material_property float4 _ColorVoxelize

#pragma pk_multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#pragma pk_program SHADER_STAGE_FRAGMENT ForwardFs
#pragma pk_program SHADER_STAGE_FRAGMENT GBufferFs PK_META_PASS_GBUFFER
#pragma pk_program SHADER_STAGE_FRAGMENT GIVoxelizeFs PK_META_PASS_GIVOXELIZE

#define PK_MESHLET_USE_FUNC_CULL 1

#include "includes/GBuffers.glsl"
#include "includes/Meshlets.glsl"

#if defined(PK_META_PASS_GBUFFER)
PK_DECLARE_VS_ATTRIB(float3 vs_Normal);
#endif

#if defined(SHADER_STAGE_MESH_TASK)

bool PK_MESHLET_FUNC_CULL(const PKMeshlet meshlet)
{
    return Meshlet_Cone_Cull(meshlet, pk_ViewWorldOrigin.xyz);
}

#elif defined(SHADER_STAGE_MESH_ASSEMBLY)

void PK_MESHLET_FUNC_VERTEX(uint vertex_index, PKVertex vertex, inout float4 sv_Position)
{
    sv_Position = ObjectToClipPos(vertex.position);
#if defined(PK_META_PASS_GBUFFER)
    vs_Normal[vertex_index] = ObjectToWorldVec(vertex.normal);
#endif
}

#elif defined(SHADER_STAGE_FRAGMENT)

#include "includes/SceneGIVX.glsl"

[pk_local(ForwardFs, GBufferFs)] out float4 SV_Target0;

void ForwardFs() { SV_Target0 = _Color; }
void GBufferFs() { SV_Target0 = EncodeGBufferWorldNR(vs_Normal, 0.0f, 0.0f); }
void GIVoxelizeFs() { GI_Store_Voxel(GI_FragVoxelToWorldSpace(gl_FragCoord.xyz), _ColorVoxelize); }

#endif
