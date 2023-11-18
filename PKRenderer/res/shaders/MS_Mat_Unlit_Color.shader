#Cull Back
#ZTest LEqual
#ZWrite Off

#MaterialProperty float4 _Color
#MaterialProperty float4 _ColorVoxelize

#multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#include includes/GBuffers.glsl
#include includes/Meshlets.glsl

#pragma PROGRAM_MESH_TASK

bool PK_IS_VISIBLE_MESHLET(const PKMeshlet meshlet)
{
    return true;
}

#pragma PROGRAM_MESH_ASSEMBLY

#if defined(PK_META_PASS_GBUFFER)
out float3 vs_Normal[];
#endif

void PK_MESHLET_ASSIGN_VERTEX_OUTPUTS(uint vertexIndex, PKVertex vertex, inout float4 sv_Position)
{
    sv_Position = ObjectToClipPos(vertex.position);
#if defined(PK_META_PASS_GBUFFER)
    vs_Normal[vertexIndex] = ObjectToWorldVec(vertex.normal);
#endif
}

#pragma PROGRAM_FRAGMENT
#if defined(PK_META_PASS_GBUFFER)
in float3 vs_Normal;
#endif
out float4 SV_Target0;

void main()
{
#if defined(PK_META_PASS_GBUFFER)
    SV_Target0 = EncodeGBufferWorldNR(vs_Normal, 0.0f, 0.0f);
#elif defined(PK_META_PASS_GIVOXELIZE)
    SV_Target0 = _ColorVoxelize;
#else
    SV_Target0 = _Color;
#endif
}