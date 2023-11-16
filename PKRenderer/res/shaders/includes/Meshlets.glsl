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

PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint2, pk_Meshlet_Tasklets, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Submeshes, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlets, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Vertices, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(byte, pk_Meshlet_Indices, PK_SET_PASS);

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

// first uint4 of packed meshlet
struct PKMeshletLite
{
    uint vertexFirst;
    uint triangleFirst;
    uint vertexCount;
    uint triangleCount;
};

// packed as 2x uint4
struct PKMeshlet
{
    uint vertexFirst;
    uint triangleFirst;
    float3 coneAxis;
    float coneCutoff;
    uint vertexCount;
    uint triangleCount;
    float3 coneApex;
    float3 center;
    float3 extents;
};

// Based on PK::Assets::Mesh::Meshlet
struct PKVertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texcoord;
};

// Quaternion multiplication
// http://mathworld.wolfram.com/Quaternion.html
float4 Meshlet_QuaternionMul(float4 q1, float4 q2)
{
    return float4(q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz), q1.w * q2.w - dot(q1.xyz, q2.xyz));
}

// Vector rotation with a quaternion
// http://mathworld.wolfram.com/Quaternion.html
float3 Meshlet_QuaternionMulVector(float4 q, float3 v)
{
    float4 qc = q * float4(-1, -1, -1, 1);
    return Meshlet_QuaternionMul(q, Meshlet_QuaternionMul(float4(v, 0), qc)).xyz;
}

// Unpacking functions 
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

PKMeshletLite Meshlet_Unpack_MeshletLite(const uint4 packed)
{
    PKMeshletLite m;
    m.vertexFirst = packed.x;
    m.triangleFirst = packed.y;
    m.vertexCount = bitfieldExtract(packed.w, 0, 8);
    m.triangleCount = bitfieldExtract(packed.w, 8, 8);
    return m;
}

PKMeshlet Meshlet_Unpack_Meshlet(const uint4 packed0, const uint4 packed1)
{
    PKMeshlet m;
    m.vertexFirst = packed0.x;
    m.triangleFirst = packed0.y;
    m.coneAxis = unpackSnorm4x8(packed0.z).xyz;
    m.coneCutoff = unpackUnorm4x8(packed0.z).w;
    m.vertexCount = bitfieldExtract(packed0.w, 0, 8);
    m.triangleCount = bitfieldExtract(packed0.w, 8, 8);
    m.coneApex.x = unpackHalf2x16(packed0.w).y;
    m.coneApex.yz = unpackHalf2x16(packed1.x);
    m.center.xy = unpackHalf2x16(packed1.y);
    m.center.z = unpackHalf2x16(packed1.z).x;
    m.extents.x = unpackHalf2x16(packed1.z).y;
    m.extents.yz = unpackHalf2x16(packed1.w);
    return m;
}

PKVertex Meshlet_Unpack_Vertex(const uint4 packed, const float3 smbbmin, const float3 smbbmax)
{
    PKVertex v;
    v.position.xy = unpackUnorm2x16(packed.x);
    v.position.z = unpackUnorm2x16(packed.y).x;
    v.tangent.w = bitfieldExtract(packed.y, 28, 1) == 0u ? -1.0f : 1.0f;
    v.texcoord = unpackHalf2x16(packed.z);

    // Decode tangent space rotation
    float4 quat;
    {
        const uint swizzleIndex = bitfieldExtract(packed.w, 30, 2);
        const float4 swizzledquat = float4
        (
            1.0f,
            (bitfieldExtract(packed.w, 0,  10) / 1023.0f) * 2.0f - 1.0f,
            (bitfieldExtract(packed.w, 10, 10) / 1023.0f) * 2.0f - 1.0f,
            (bitfieldExtract(packed.w, 20, 10) / 1023.0f) * 2.0f - 1.0f
        );

        quat = swizzledquat;
        quat = lerp(quat, swizzledquat.wxyz, bool(swizzleIndex == 1).xxxx);
        quat = lerp(quat, swizzledquat.zwxy, bool(swizzleIndex == 2).xxxx);
        quat = lerp(quat, swizzledquat.yzwx, bool(swizzleIndex == 3).xxxx);
        quat = normalize(quat);
    }

    v.normal = Meshlet_QuaternionMulVector(quat, float3(0,0,1));
    v.tangent.xyz = Meshlet_QuaternionMulVector(quat, float3(1,0,0));
    v.position = lerp(smbbmin, smbbmax, v.position);
    return v;
}

// Loading functions
PKTasklet Meshlet_Load_Tasklet(const uint taskIndex)
{
    return Meshlet_Unpack_Tasklet(PK_BUFFER_DATA(pk_Meshlet_Tasklets, taskIndex));
}

PKSubmesh Meshlet_Load_Submesh(const uint index) 
{ 
    const uint4 packed0 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 2u + 0u);
    const uint4 packed1 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 2u + 1u);
    return Meshlet_Unpack_Submesh(packed0, packed1);
}

PKMeshletLite Meshlet_Load_MeshletLite(const uint index)
{
    return Meshlet_Unpack_MeshletLite(PK_BUFFER_DATA(pk_Meshlets, index * 2u + 0u));
}

PKMeshlet Meshlet_Load_Meshlet(const uint index)
{
    uint4 packed0 = PK_BUFFER_DATA(pk_Meshlets, index * 2u + 0u);
    uint4 packed1 = PK_BUFFER_DATA(pk_Meshlets, index * 2u + 1u);
    return Meshlet_Unpack_Meshlet(packed0, packed1);
}

PKVertex Meshlet_Load_Vertex(const uint index, const float3 smbbmin, const float3 smbbmax)
{
    return Meshlet_Unpack_Vertex(PK_BUFFER_DATA(pk_Meshlet_Vertices, index), smbbmin, smbbmax);
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

        const uint meshletFirst = payload.packed0.w;
        const uint instanceId = payload.packed1.w;
        meshletCount = subgroupBroadcastFirst(meshletCount);
    
        const uint meshletLocalIndex = gl_LocalInvocationID.x;
        const uint meshletIndex = meshletFirst + meshletLocalIndex;
        const PKMeshlet meshlet = Meshlet_Load_Meshlet(meshletIndex);
        
        const float  coneClip = step(dot(meshlet.coneAxis, meshlet.coneAxis), 0.01f);
        const float3 coneAxis = safeNormalize(ObjectToWorldVec(meshlet.coneAxis)) * coneClip;
        const float3 coneApex = ObjectToWorldPos(meshlet.coneApex);
        const float3 coneView = normalize(coneApex - pk_ViewWorldOrigin.xyz);
        
        bool isVisible = true;
        isVisible = isVisible && meshletLocalIndex < meshletCount;
        isVisible = isVisible && dot(coneView, coneAxis) < meshlet.coneCutoff;
        isVisible = isVisible && PK_IS_VISIBLE_MESHLET(meshlet);
    
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

    void PK_MESHLET_ASSIGN_VERTEX_OUTPUTS(uint vertexIndex, PKVertex vertex, inout float4 sv_Position);

    taskPayloadSharedEXT PKMeshTaskPayload payload;

    layout(local_size_x = MESHLET_LOCAL_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;
    layout(triangles, max_vertices = MAX_VERTICES_PER_MESHLET, max_primitives = MAX_TRIANGLES_PER_MESHLET) out;
    void main()
    {
        const uint meshletFirst = payload.packed0.w;
        const uint instanceId = payload.packed1.w;

        const uint meshletIndex = meshletFirst + payload.deltaIDs[gl_WorkGroupID.x];
        const PKMeshletLite meshlet = Meshlet_Load_MeshletLite(meshletIndex);
        
        PK_INSTANCING_ASSIGN_STAGE_LOCALS_MANUAL(instanceId);

        const uint vertexFirst = meshlet.vertexFirst;
        const uint vertexCount = meshlet.vertexCount;
        const uint triangleFirst = meshlet.triangleFirst;
        const uint triangleCount = meshlet.triangleCount;
        const float3 smbbmin = uintBitsToFloat(payload.packed0.xyz);
        const float3 smbbmax = uintBitsToFloat(payload.packed1.xyz);

        SetMeshOutputsEXT(vertexCount, triangleCount);

        [[loop]]
        for (uint i = 0u; i < TRIANGLES_PER_MESHLET_THREAD; ++i)
        {
            const uint triangleIndex = gl_LocalInvocationID.x * TRIANGLES_PER_MESHLET_THREAD + i;
            const uint3 indices = uint3
            (
                PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 0u),
                PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 1u),
                PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 2u)
            );
            
            [[branch]]
            if (triangleIndex < triangleCount)
            {
                gl_PrimitiveTriangleIndicesEXT[triangleIndex] = indices;
            }
        }        

        [[loop]]
        for (uint i = 0u; i < VERTICES_PER_MESHLET_THREAD; ++i)
        {
            const uint vertexIndex = gl_LocalInvocationID.x * VERTICES_PER_MESHLET_THREAD + i;
            const PKVertex vertex = Meshlet_Load_Vertex(vertexFirst + vertexIndex, smbbmin, smbbmax);
            float4 sv_Position;

            [[branch]]
            if (vertexIndex < vertexCount)
            {
                PK_SET_VERTEX_INSTANCE_ID(vertexIndex, instanceId);
                PK_MESHLET_ASSIGN_VERTEX_OUTPUTS(vertexIndex, vertex, sv_Position);
                gl_MeshVerticesEXT[vertexIndex].gl_Position = sv_Position;
            }
        }
    }
#endif