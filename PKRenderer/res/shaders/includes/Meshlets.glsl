#pragma once

#include Common.glsl
#include Encoding.glsl

#define MAX_VERTICES_PER_MESHLET 64u
#define MAX_TRIANGLES_PER_MESHLET 124u
#define MAX_MESHLETS_PER_TASK 32u
#define MAX_TASK_WORK_GROUPS 2047u
#define MESHLET_LOCAL_GROUP_SIZE 32u
#define TRIANGLES_PER_MESHLET_THREAD 4u
#define VERTICES_PER_MESHLET_THREAD 2u
#define CONE_CULL_BIAS 0.3f

PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint2, pk_Meshlet_Tasklets, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Submeshes, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlets, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Vertices, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(byte, pk_Meshlet_Indices, PK_SET_PASS);

#define MP_MATERIAL instance.x
#define MP_TRANSFORM instance.y
#define MP_FIRSTMESHLET instance.z
#define MP_USERDATA instance.w

struct PKMeshTaskPayload
{
    uint4 packed0;
    uint4 packed1;
    byte deltaIDs[MAX_MESHLETS_PER_TASK];
};

// packed as uint2
struct PKTasklet
{
    uint meshletFirst;
    uint instanceId;
    uint meshletCount;
};

// packed as 3x uint4
struct PKSubmesh
{
    float3 bbmin;
    uint meshletFirst;
    float3 bbmax;
    uint meshletCount;
};

// packed as 2x uint4
struct PKMeshlet
{
    uint vertexFirst;
    uint triangleFirst;
    uint vertexCount;
    uint triangleCount;
    float3 bbmin;
    float3 bbmax;
    float3 coneAxis;
    float3 coneApex;
    float coneCutoff;
};

// Based on PK::Assets::Mesh::Meshlet
struct PKVertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texcoord;
};

PKTasklet Meshlet_Unpack_Tasklet(uint2 packed)
{
    PKTasklet t;
    t.meshletFirst = packed.x;
    t.instanceId = bitfieldExtract(packed.y, 0, 24);
    t.meshletCount = bitfieldExtract(packed.y, 24, 8);
    return t;
}

PKSubmesh Meshlet_Unpack_Submesh(const uint4 packed0, const uint4 packed1)
{
    PKSubmesh s;
    s.bbmin = uintBitsToFloat(packed0.xyz);
    s.meshletFirst = packed0.w;
    s.bbmax = uintBitsToFloat(packed1.xyz);
    s.meshletCount = packed1.w;
    return s;
}

PKMeshlet Meshlet_Unpack_Meshlet(const uint4 packed0, const uint4 packed1, const float3 smbbmin, const float3 smbbmax)
{
    PKMeshlet m;
    m.vertexFirst = packed0.x;
    m.triangleFirst = packed0.y;
    m.bbmin.xy = unpackUnorm2x16(packed0.z);
    m.bbmin.z = unpackUnorm2x16(packed0.w).x;
    m.coneCutoff = unpackSnorm2x16(packed0.w).y;
    m.bbmax.xy = unpackUnorm2x16(packed1.x);
    m.bbmax.z = unpackUnorm2x16(packed1.y).x;
    m.coneApex.x = unpackUnorm2x16(packed1.y).y;
    m.coneApex.yz = unpackUnorm2x16(packed1.z);
    m.coneAxis.xy = unpackUnorm4x8(packed1.w).xy;
    m.vertexCount = bitfieldExtract(packed1.w, 16, 8);
    m.triangleCount = bitfieldExtract(packed1.w, 24, 8);

    m.bbmin = lerp(smbbmin, smbbmax, m.bbmin);
    m.bbmax = lerp(smbbmin, smbbmax, m.bbmax);
    m.coneApex = lerp(m.bbmin, m.bbmax, m.coneApex);
    m.coneAxis = OctaDecode(m.coneAxis.xy);
    return m;
}

PKVertex Meshlet_Unpack_Vertex(const uint4 packed, const float3 bbmin, const float3 bbmax)
{
    PKVertex v;
    v.position.xy = unpackUnorm2x16(packed.x);
    v.position.z = unpackUnorm2x16(packed.y).x;
    v.texcoord.x = bitfieldExtract(packed.y, 16, 12) / 4095.0f;
    v.texcoord.y = (bitfieldExtract(packed.y, 28, 4) | (bitfieldExtract(packed.z, 0, 8) << 4u)) / 4095.0f;
    v.normal.x = bitfieldExtract(packed.z, 8, 12) / 4095.0f;
    v.normal.y = bitfieldExtract(packed.z, 20, 12) / 4095.0f;
    v.tangent.x = bitfieldExtract(packed.w, 0, 15) / 32767.0f;
    v.tangent.y = bitfieldExtract(packed.w, 15, 15) / 32767.0f;
    v.tangent.w = bitfieldExtract(packed.w, 30, 3) == 0 ? -1.0f : 1.0f;

    v.position = lerp(bbmin, bbmax, v.position);
    v.normal = OctaDecode(v.normal.xy);
    v.tangent.xyz = OctaDecode(v.tangent.xy);
    return v;
}

PKSubmesh Meshlet_Load_Submesh(const uint index) 
{ 
    const uint4 packed0 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 2u + 0u);
    const uint4 packed1 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 2u + 1u);
    return Meshlet_Unpack_Submesh(packed0, packed1);
}

PKMeshlet Meshlet_Load_Meshlet(const uint index, const float3 smbbmin, const float3 smbbmax)
{
    uint4 packed0 = PK_BUFFER_DATA(pk_Meshlets, index * 2u + 0u);
    uint4 packed1 = PK_BUFFER_DATA(pk_Meshlets, index * 2u + 1u);
    return Meshlet_Unpack_Meshlet(packed0, packed1, smbbmin, smbbmax);
}

PKVertex Meshlet_Load_Vertex(const uint index, const float3 bbmin, const float3 bbmax)
{
    return Meshlet_Unpack_Vertex(PK_BUFFER_DATA(pk_Meshlet_Vertices, index), bbmin, bbmax);
}

PKTasklet Meshlet_Load_Tasklet(const uint taskIndex)
{
    return Meshlet_Unpack_Tasklet(PK_BUFFER_DATA(pk_Meshlet_Tasklets, taskIndex));
}

#if defined(SHADER_STAGE_MESH_TASK)

    bool PK_IS_VISIBLE_MESHLET(const PKMeshlet meshlet);
    
    PK_DECLARE_LOCAL_CBUFFER(pk_Meshlet_DispatchOffset)
    {
        uint firstTasklet;
    };
    
    taskPayloadSharedEXT PKMeshTaskPayload payload;
    
    layout(local_size_x = MAX_MESHLETS_PER_TASK) in;
    void main()
    {
        const uint taskId = firstTasklet + gl_WorkGroupID.x;
        uint meshletCount;
        
        [[branch]]
        if (subgroupElect())
        {
            const PKTasklet tasklet = Meshlet_Load_Tasklet(taskId);
            PK_INSTANCING_ASSIGN_STAGE_LOCALS_MANUAL(tasklet.instanceId);
            const PKSubmesh submesh = Meshlet_Load_Submesh(pk_Instancing_Submesh);
            payload.packed0.xyz = floatBitsToUint(submesh.bbmin);
            payload.packed1.xyz = floatBitsToUint(submesh.bbmax);
            payload.packed0.w = tasklet.meshletFirst;
            payload.packed1.w = tasklet.instanceId;
            meshletCount = tasklet.meshletCount;
        }
    
        subgroupBarrier();

        const float3 smbbmin = uintBitsToFloat(payload.packed0.xyz);
        const float3 smbbmax = uintBitsToFloat(payload.packed1.xyz);
        const uint meshletFirst = payload.packed0.w;
        const uint instanceId = payload.packed1.w;
        meshletCount = subgroupBroadcastFirst(meshletCount);
    
        const uint meshletLocalIndex = gl_LocalInvocationID.x;
        const uint meshletIndex = meshletFirst + meshletLocalIndex;
        const PKMeshlet meshlet = Meshlet_Load_Meshlet(meshletIndex, smbbmin, smbbmax);
        
        const float3 coneAxis = ObjectToWorldDir(meshlet.coneAxis);
        const float3 coneApex = ObjectToWorldPos(meshlet.coneApex);
        const float3 coneView = normalize(coneApex - pk_ViewWorldOrigin.xyz);
        
        bool isVisible = true;
        isVisible = isVisible && meshletLocalIndex < meshletCount;
        isVisible = isVisible && PK_IS_VISIBLE_MESHLET(meshlet);
        //@TODO this is unrealiable with low poly meshes.
       // isVisible = isVisible && dot(coneView, coneAxis) < (meshlet.coneCutoff + CONE_CULL_BIAS);
    
        uint4 visibleMask = subgroupBallot(isVisible);
        uint visibleCount = subgroupBallotBitCount(visibleMask);
        uint deltaIndex = subgroupBallotExclusiveBitCount(visibleMask);
    
        if (isVisible)
        {
            payload.deltaIDs[deltaIndex] = byte(meshletLocalIndex);
        }
    
        EmitMeshTasksEXT(visibleCount, 1, 1);
    }

#elif defined(SHADER_STAGE_MESH_ASSEMBLY)
    void PK_MESHLET_ASSIGN_VERTEX_OUTPUTS(uint vertexIndex, PKVertex vertex);

    taskPayloadSharedEXT PKMeshTaskPayload payload;
    
    layout(local_size_x = MESHLET_LOCAL_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;
    layout(triangles, max_vertices = MAX_VERTICES_PER_MESHLET, max_primitives = MAX_TRIANGLES_PER_MESHLET) out;
    void main()
    {
        const uint meshletFirst = payload.packed0.w;
        const uint instanceId = payload.packed1.w;
        const float3 smbbmin = uintBitsToFloat(payload.packed0.xyz);
        const float3 smbbmax = uintBitsToFloat(payload.packed1.xyz);
        
        uint vertexFirst;
        uint vertexCount;
        uint triangleFirst;
        uint triangleCount;
        float3 bbmin;
        float3 bbmax;

        [[branch]]
        if (subgroupElect())
        {
            const uint meshletIndex = meshletFirst + uint(payload.deltaIDs[gl_WorkGroupID.x]);
            const PKMeshlet meshlet = Meshlet_Load_Meshlet(meshletIndex, smbbmin, smbbmax);
            
            PK_INSTANCING_ASSIGN_STAGE_LOCALS_MANUAL(instanceId);

            vertexFirst = meshlet.vertexFirst;
            vertexCount = meshlet.vertexCount;
            triangleFirst = meshlet.triangleFirst;
            triangleCount = meshlet.triangleCount;
            bbmin = meshlet.bbmin;
            bbmax = meshlet.bbmax;
        }
        
        subgroupBarrier();
        
        vertexFirst = subgroupBroadcastFirst(vertexFirst);
        vertexCount = subgroupBroadcastFirst(vertexCount);
        triangleFirst = subgroupBroadcastFirst(triangleFirst);
        triangleCount = subgroupBroadcastFirst(triangleCount);
        bbmin = subgroupBroadcastFirst(bbmin);
        bbmax = subgroupBroadcastFirst(bbmax);

        SetMeshOutputsEXT(vertexCount, triangleCount);
        
        [[unroll]]
        for (uint i = 0u; i < VERTICES_PER_MESHLET_THREAD; ++i)
        {
            const uint vertexIndex = gl_LocalInvocationID.x * VERTICES_PER_MESHLET_THREAD + i;
        
            [[branch]]
            if (vertexIndex < vertexCount)
            {
                const PKVertex vertex = Meshlet_Load_Vertex(vertexFirst + vertexIndex, bbmin, bbmax);
                PK_SET_VERTEX_INSTANCE_ID(vertexIndex, instanceId);
                PK_MESHLET_ASSIGN_VERTEX_OUTPUTS(vertexIndex, vertex);
            }
        }
        
        [[unroll]]
        for (uint i = 0u; i < TRIANGLES_PER_MESHLET_THREAD; ++i)
        {
            const uint triangleIndex = gl_LocalInvocationID.x * TRIANGLES_PER_MESHLET_THREAD + i;
            
            [[branch]]
            if (triangleIndex < triangleCount)
            {
                gl_PrimitiveTriangleIndicesEXT[triangleIndex] = uint3
                (
                    PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 0u),
                    PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 1u),
                    PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 2u)
                );
            }
        }
    }
#endif