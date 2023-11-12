#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_vote : require

#WithAtomicCounter

#ZTest GEqual
#ZWrite False
#Cull Back
#LogVerbose

#include includes/Meshlets.glsl

// @TODO NOTE that for optimization purposes PK_Draw uses uint4(material, transform, global meshlet submesh, userdata) internally (this saves a redundant struct descriptor)
#define PK_Draw uint4
layout(std430, set = 3) readonly restrict buffer pk_Instancing_Transforms { mat3x4 pk_Instancing_Transforms_Data[]; };
layout(std430, set = 3) readonly restrict buffer pk_Instancing_Indices { PK_Draw pk_Instancing_Indices_Data[]; };

#pragma PROGRAM_MESH_TASK

shared float3x4 lds_LocalToWorld;
shared PKTasklet lds_Tasklet;
taskPayloadSharedEXT PKMeshTaskPayload payload;

// Write buffer that has instance id & meshlet offsets per workgroup. read here

layout(local_size_x = MAX_MESHLETS_PER_TASK) in;
void main()
{
    [[branch]]
    if (gl_LocalInvocationID.x == 0)
    {
        uint taskId = PK_AtomicCounterNext();
        // @TODO get this from culling
        lds_Tasklet = Meshlet_Load_Tasklet(taskId);
        PK_Draw draw = pk_Instancing_Indices_Data[lds_Tasklet.instanceId];
        payload.instance = uint4(draw.xy, lds_Tasklet.firstMeshlet, draw.w);
        lds_LocalToWorld = pk_Instancing_Transforms_Data[payload.MP_TRANSFORM];
    }

    barrier();

    const uint meshletLocalIndex = gl_LocalInvocationID.x;
    const uint meshletIndex = lds_Tasklet.firstMeshlet + meshletLocalIndex;
    const PKMeshlet meshlet = Meshlet_Load_Meshlet(meshletIndex);
    
    const float3 coneAxis = float4(OctaDecode(meshlet.coneAxis), 0.0f) * lds_LocalToWorld;
    const float3 coneApex = float4(meshlet.coneApex, 1.0f) * lds_LocalToWorld;
    const float3 coneView = normalize(coneApex - pk_ViewWorldOrigin.xyz);
    
    bool isVisible = true;
    isVisible = isVisible && meshletLocalIndex < lds_Tasklet.meshletCount;
   // isVisible = isVisible && dot(coneView, coneAxis) < (meshlet.coneCutoff + CONE_CULL_BIAS);

    uint4 visibleMask = subgroupBallot(isVisible);
    uint visibleCount = subgroupBallotBitCount(visibleMask);
    uint deltaIndex = subgroupBallotExclusiveBitCount(visibleMask);

    // @TODO add culling
    if (isVisible)
    {
        payload.deltaIDs[deltaIndex] = byte(meshletLocalIndex);
    }

    EmitMeshTasksEXT(visibleCount, 1, 1);
}

#pragma PROGRAM_MESH_ASSEMBLY

shared float3x4 lds_LocalToWorld;
shared PKMeshlet lds_meshlet;
taskPayloadSharedEXT PKMeshTaskPayload payload;

out float3 vs_COLOR[];

layout(local_size_x = MESHLET_LOCAL_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = MAX_VERTICES_PER_MESHLET, max_primitives = MAX_TRIANGLES_PER_MESHLET) out;
void main()
{
    const uint meshletIndex = payload.MP_FIRSTMESHLET + uint(payload.deltaIDs[gl_WorkGroupID.x]);

    [[branch]]
    if (gl_LocalInvocationID.x == 0)
    {
        lds_LocalToWorld = pk_Instancing_Transforms_Data[payload.MP_TRANSFORM];
        lds_meshlet = Meshlet_Load_Meshlet(meshletIndex);
    }

    barrier();
    
    SetMeshOutputsEXT(lds_meshlet.vertexCount, lds_meshlet.triangleCount);

    [[unroll]]
    for (uint i = 0u; i < VERTICES_PER_MESHLET_THREAD; ++i)
    {
        const uint vertexIndex = gl_LocalInvocationID.x * VERTICES_PER_MESHLET_THREAD + i;

        [[branch]]
        if (vertexIndex < lds_meshlet.vertexCount)
        {
            const PKVertex vertex = Meshlet_Load_Vertex(lds_meshlet.firstVertex + vertexIndex, lds_meshlet.center, lds_meshlet.radius);
            gl_MeshVerticesEXT[vertexIndex].gl_Position = WorldToClipPos(float4(vertex.position, 1.0f) * lds_LocalToWorld);
            vs_COLOR[vertexIndex] = HSVToRGB((meshletIndex % 32u) / 32.0f, 1.0f, 1.0f);
        }
    }

    [[unroll]]
    for (uint i = 0u; i < TRIANGLES_PER_MESHLET_THREAD; ++i)
    {
        const uint triangleIndex = gl_LocalInvocationID.x * TRIANGLES_PER_MESHLET_THREAD + i;

        [[branch]]
        if (triangleIndex < lds_meshlet.triangleCount)
        {
            gl_PrimitiveTriangleIndicesEXT[triangleIndex] = uint3
            (
                PK_BUFFER_DATA(pk_Meshlet_Indices, lds_meshlet.firstTriangle * 3u + triangleIndex * 3u + 0u),
                PK_BUFFER_DATA(pk_Meshlet_Indices, lds_meshlet.firstTriangle * 3u + triangleIndex * 3u + 1u),
                PK_BUFFER_DATA(pk_Meshlet_Indices, lds_meshlet.firstTriangle * 3u + triangleIndex * 3u + 2u)
            );
        }
    }
}

#pragma PROGRAM_FRAGMENT

in float3 vs_COLOR;
layout(location = 0) out float4 SV_Target0;

void main()
{
    SV_Target0 = float4(vs_COLOR, 1.0f);
}