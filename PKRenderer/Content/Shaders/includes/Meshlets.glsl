#pragma once

#extension GL_KHR_shader_subgroup_arithmetic : require

#pragma pk_program SHADER_STAGE_MESH_TASK MainMeshTask
#pragma pk_program SHADER_STAGE_MESH_ASSEMBLY MainMeshAssembly

#include "Common.glsl"
#include "Encoding.glsl"

#define MAX_VERTICES_PER_MESHLET 64u
#define MAX_TRIANGLES_PER_MESHLET 124u
#define MAX_MESHLETS_PER_TASK 32u
#define MESHLET_LOCAL_GROUP_SIZE 32u
#define TRIANGLES_PER_MESHLET_THREAD 4u
#define VERTICES_PER_MESHLET_THREAD 2u
#define MESHLET_UINT4_STRIDE 3u
#define MESHLET_MAX_ERROR PK_HALF_MAX_MINUS1

#ifndef PK_MESHLET_MAX_LOD_ONLY
    #define PK_MESHLET_MAX_LOD_ONLY 0
#endif

#ifndef PK_MESHLET_LOD_ERROR_THRESHOLD
    #define PK_MESHLET_LOD_ERROR_THRESHOLD 0.15f
#endif

#ifndef PK_MESHLET_HAS_EXTRA_PAYLOAD_DATA
    #define PK_MESHLET_HAS_EXTRA_PAYLOAD_DATA 0
#endif

#ifndef PK_MESHLET_USE_FUNC_CULL
    #define PK_MESHLET_USE_FUNC_CULL 0
#endif

#ifndef PK_MESHLET_USE_FUNC_TASKLET
    #define PK_MESHLET_USE_FUNC_TASKLET 0
#endif

#ifndef PK_MESHLET_USE_FUNC_TRIANGLE
    #define PK_MESHLET_USE_FUNC_TRIANGLE 0
#endif

PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint2, pk_Meshlet_Tasklets, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Submeshes, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlets, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Vertices, PK_SET_PASS);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(byte, pk_Meshlet_Indices, PK_SET_PASS);

struct PKMeshTaskPayload
{
    uint4 packed0;
    uint4 packed1;
    uint4 visibleMask; 

#if PK_MESHLET_HAS_EXTRA_PAYLOAD_DATA == 1
    PK_MESHLET_EXTRA_PAYLOAD_DATA extra;
#endif
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

    float4 lodCenterErrorCurrent;
    float4 lodCenterErrorParent;
};

// Based on PKAssets::PKMeshletVertex
struct PKVertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texcoord;
};

float3 Meshlet_QuaternionMulVector(float4 q, float3 v)
{
    const float3 t = 2 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
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

PKMeshlet Meshlet_Unpack_Meshlet(const uint4 packed0, const uint4 packed1, const uint4 packed2)
{
    PKMeshlet m;
    m.vertexFirst = packed0.x;
    m.triangleFirst = packed0.y;
    m.coneAxis = unpackSnorm4x8(packed0.z).xyz;
    m.coneCutoff = unpackSnorm4x8(packed0.z).w;
    m.vertexCount = bitfieldExtract(packed0.w, 0, 8);
    m.triangleCount = bitfieldExtract(packed0.w, 8, 8);
    m.coneApex.x = unpackHalf2x16(packed0.w).y;
    m.coneApex.yz = unpackHalf2x16(packed1.x);
    m.center.xy = unpackHalf2x16(packed1.y);
    m.center.z = unpackHalf2x16(packed1.z).x;
    m.extents.x = unpackHalf2x16(packed1.z).y;
    m.extents.yz = unpackHalf2x16(packed1.w);

    m.lodCenterErrorCurrent.xy = unpackHalf2x16(packed2.x);
    m.lodCenterErrorCurrent.zw = unpackHalf2x16(packed2.y);
    m.lodCenterErrorParent.xy = unpackHalf2x16(packed2.z);
    m.lodCenterErrorParent.zw = unpackHalf2x16(packed2.w);

    return m;
}

PKVertex Meshlet_Unpack_Vertex(const uint4 packed, const float3 smbbmin, const float3 smbbmax)
{
    PKVertex v;
    v.position.xy = unpackUnorm2x16(packed.x);
    v.position.z = unpackUnorm2x16(packed.y).x;
    v.position = lerp(smbbmin, smbbmax, v.position);
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
    v.tangent.w = bitfieldExtract(packed.y, 28, 1) * 2.0f - 1.0f;
    return v;
}

// Loading functions
PKTasklet Meshlet_Load_Tasklet(const uint taskIndex) { return Meshlet_Unpack_Tasklet(PK_BUFFER_DATA(pk_Meshlet_Tasklets, taskIndex)); }

PKSubmesh Meshlet_Load_Submesh(const uint index) 
{ 
    return Meshlet_Unpack_Submesh(
        PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 2u + 0u), 
        PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 2u + 1u)); 
}

PKMeshletLite Meshlet_Load_MeshletLite(const uint index) { return Meshlet_Unpack_MeshletLite(PK_BUFFER_DATA(pk_Meshlets, index * MESHLET_UINT4_STRIDE + 0u)); }

PKMeshlet Meshlet_Load_Meshlet(const uint index) 
{ 
    return Meshlet_Unpack_Meshlet(
        PK_BUFFER_DATA(pk_Meshlets, index * MESHLET_UINT4_STRIDE + 0u), 
        PK_BUFFER_DATA(pk_Meshlets, index * MESHLET_UINT4_STRIDE + 1u),
        PK_BUFFER_DATA(pk_Meshlets, index * MESHLET_UINT4_STRIDE + 2u)); 
}

PKVertex Meshlet_Load_Vertex(const uint index, const float3 smbbmin, const float3 smbbmax) { return Meshlet_Unpack_Vertex(PK_BUFFER_DATA(pk_Meshlet_Vertices, index), smbbmin, smbbmax); }

#if defined(SHADER_STAGE_MESH_TASK)

    bool Meshlet_Cull_Lod(const PKMeshlet meshlet)
    {
        #if PK_MESHLET_MAX_LOD_ONLY == 1
            return meshlet.lodCenterErrorParent.w > MESHLET_MAX_ERROR;
        #else
            const float threshold = PK_MESHLET_LOD_ERROR_THRESHOLD * pk_MeshletCullParams.x;
            const float4 centerC = ObjectToClipPos(meshlet.lodCenterErrorCurrent.xyz);
            const float4 centerP = ObjectToClipPos(meshlet.lodCenterErrorParent.xyz);
            float errorC = pk_Instancing_UniformScale * meshlet.lodCenterErrorCurrent.w;
            float errorP = pk_Instancing_UniformScale * meshlet.lodCenterErrorParent.w;
            errorC *= inversesqrt(max(1e-6f, dot(centerC, centerC) - pow2(errorC)));
            errorP *= inversesqrt(max(1e-6f, dot(centerP, centerP) - pow2(errorP)));
            return errorP > threshold && errorC <= threshold;
        #endif 
    }

    bool Meshlet_Frustum_Perspective_Cull(const PKMeshlet meshlet, float4x4 worldToClip)
    {
        const float4 deltaX = (2.0f * meshlet.extents.x) * (worldToClip * float4(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0], 0.0f));
        const float4 deltaY = (2.0f * meshlet.extents.y) * (worldToClip * float4(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1], 0.0f));
    	const float4 deltaZ = (2.0f * meshlet.extents.z) * (worldToClip * float4(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2], 0.0f));
    	const float4 clipZ0 = worldToClip * float4(ObjectToWorldPos(meshlet.center - meshlet.extents), 1.0f);
    	const float4 clipZ1 = clipZ0 + deltaZ;

        float4 pmin = 1.0f.xxxx;
    
    	for (uint i = 0; i < 4; ++i)
        {
            const float4 clipMin = clipZ0 + deltaX * (i & 0x1) + deltaY * (i / 2);
            const float4 clipMax = clipZ1 + deltaX * (i & 0x1) + deltaY * (i / 2);
            pmin = min(pmin, float4(clipMin.xy, -clipMin.xy) - clipMin.w);
            pmin = min(pmin, float4(clipMax.xy, -clipMax.xy) - clipMax.w);
        }
    
    	return All_Less(pmin, 0.0f.xxxx);
    }

    bool Meshlet_Frustum_Ortho_Cull(const PKMeshlet meshlet, float4x4 worldToClip)
    {
    	const float3 clipCenter = (worldToClip * float4(ObjectToWorldPos(meshlet.center), 1.0f)).xyz;
    	const float3 clipDelta = abs(meshlet.extents.x * (worldToClip * float4(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0], 0.0f)).xyz) + 
    							 abs(meshlet.extents.y * (worldToClip * float4(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1], 0.0f)).xyz) + 
    							 abs(meshlet.extents.z * (worldToClip * float4(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2], 0.0f)).xyz);
    								
    	const float3 rectMin = clipCenter - clipDelta;
    	const float3 rectMax = clipCenter + clipDelta;
    	return rectMax.z > 0.0f && rectMin.z < 1.0f && All_Greater(rectMax.xy, -1.0f.xx) && All_Less(rectMin.xy, 1.0f.xx);
    }

    bool Meshlet_Sphere_Cull(const PKMeshlet meshlet, float3 center, float radius)
    {
        /*
        @TODO 
        const float3 bbmin = meshlet.center - meshlet.extents;
        const float3 bbmax = meshlet.center + meshlet.extents;
        float3 wbbmin = +1e+32f.xxx;
        float3 wbbmax = -1e+32f.xxx;
    
        [[loop]]
        for (uint i = 0u; i < 8u; i++)
        {
            const bool3 useMax = bool3((i & 1) != 0, (i & 2) != 0, (i & 4) != 0);
            const float3 corner = lerp(bbmin, bbmax, useMax);
            const float3 pos = ObjectToWorldPos(corner);
            wbbmin = min(wbbmin, pos);
            wbbmax = max(wbbmax, pos);
        }

        const float3 meshlet

        float3 d = abs(light.position.xyz - currentCell.center) - currentCell.extents;
        float r = light.radius - cmax(min(d, float3(0.0f)));
        d = max(d, float3(0.0f));
        return light.radius > 0.0f && dot(d, d) <= r * r;
        */
        return true;
    }

    bool Meshlet_Cone_Cull(const PKMeshlet meshlet, float3 cullOrigin)
    {
        const float3 coneAxis = safeNormalize(ObjectToWorldVec(meshlet.coneAxis));
        const float3 coneApex = ObjectToWorldPos(meshlet.coneApex);
        const float3 coneView = normalize(coneApex - cullOrigin);
        return dot(coneView, coneAxis) < meshlet.coneCutoff;
    }

    bool Meshlet_Cone_Cull_Directional(const PKMeshlet meshlet, float3 cullDirection)
    {
        const float3 coneAxis = safeNormalize(ObjectToWorldVec(meshlet.coneAxis));
        const float3 coneApex = ObjectToWorldPos(meshlet.coneApex);
        return dot(cullDirection, coneAxis) < meshlet.coneCutoff;
    }

    #if PK_MESHLET_USE_FUNC_TASKLET == 1
    void PK_MESHLET_FUNC_TASKLET(inout PKMeshTaskPayload payload);
    #else
    void PK_MESHLET_FUNC_TASKLET(inout PKMeshTaskPayload payload) {}
    #endif

    #if PK_MESHLET_USE_FUNC_CULL == 1
    bool PK_MESHLET_FUNC_CULL(const PKMeshlet meshlet);
    #else
    bool PK_MESHLET_FUNC_CULL(const PKMeshlet meshlet) {}
    #endif

    PK_DECLARE_LOCAL_CBUFFER(pk_Meshlet_DispatchOffset)
    {
        uint firstTasklet;
    };
    
    taskPayloadSharedEXT PKMeshTaskPayload payload;

    layout(local_size_x = MAX_MESHLETS_PER_TASK) in;
    void MainMeshTask()
    {
        uint meshletCount;
        
        [[branch]]
        if (subgroupElect())
        {
            const uint taskId = firstTasklet + gl_WorkGroupID.x;
            const PKTasklet tasklet = Meshlet_Load_Tasklet(taskId);
            PK_INSTANCING_ASSIGN_LOCALS(tasklet.instanceId);
            const PKSubmesh submesh = Meshlet_Load_Submesh(pk_Instancing_Submesh);
            payload.packed0.xyz = floatBitsToUint(submesh.bbmin);
            payload.packed1.xyz = floatBitsToUint(submesh.bbmax);
            payload.packed0.w = tasklet.meshletFirst;
            payload.packed1.w = tasklet.instanceId;
            meshletCount = tasklet.meshletCount;
            PK_MESHLET_FUNC_TASKLET(payload);
        }
    
        subgroupBarrier();

        PK_INSTANCING_BROADCAST_LOCALS_MANUAL();

        const uint meshletFirst = payload.packed0.w;
        const uint instanceId = payload.packed1.w;
        meshletCount = subgroupBroadcastFirst(meshletCount);
    
        const uint meshletLocalIndex = gl_LocalInvocationID.x;
        const uint meshletIndex = meshletFirst + meshletLocalIndex;
        const PKMeshlet meshlet = Meshlet_Load_Meshlet(meshletIndex);
        
        bool isVisible = true;
        isVisible = isVisible && meshletLocalIndex < meshletCount;
        isVisible = isVisible && Meshlet_Cull_Lod(meshlet);
        isVisible = isVisible && PK_MESHLET_FUNC_CULL(meshlet);

        uint4 visibleMask = subgroupBallot(isVisible);
        uint visibleCount = subgroupBallotBitCount(visibleMask);
    
        payload.visibleMask = visibleMask;

        EmitMeshTasksEXT(visibleCount, 1, 1);
    }

#elif defined(SHADER_STAGE_MESH_ASSEMBLY)

    void PK_MESHLET_FUNC_VERTEX(uint vertexIndex, PKVertex vertex, inout float4 sv_Position);

    #if PK_MESHLET_USE_FUNC_TRIANGLE == 1
    void PK_MESHLET_FUNC_TRIANGLE(uint triangleIndex, inout uint3 indices);
    #else
    void PK_MESHLET_FUNC_TRIANGLE(uint triangleIndex, inout uint3 indices) {}
    #endif

    taskPayloadSharedEXT PKMeshTaskPayload payload;

    layout(local_size_x = MESHLET_LOCAL_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;
    layout(triangles, max_vertices = MAX_VERTICES_PER_MESHLET, max_primitives = MAX_TRIANGLES_PER_MESHLET) out;
    void MainMeshAssembly()
    {
        const uint meshletFirst = payload.packed0.w;
        const uint instanceId = payload.packed1.w;

        uint deltaIndex = subgroupBallotExclusiveBitCount(payload.visibleMask);
        deltaIndex = lerp(0u, gl_LocalInvocationID.x, deltaIndex == gl_WorkGroupID.x);
        deltaIndex = subgroupMax(deltaIndex);

        const uint meshletIndex = meshletFirst + deltaIndex;
        const PKMeshletLite meshlet = Meshlet_Load_MeshletLite(meshletIndex);
        
        PK_INSTANCING_ASSIGN_LOCALS(instanceId);

        const uint vertexFirst = meshlet.vertexFirst;
        const uint vertexCount = meshlet.vertexCount;
        const uint triangleFirst = meshlet.triangleFirst;
        const uint triangleCount = meshlet.triangleCount;
        const float3 smbbmin = uintBitsToFloat(payload.packed0.xyz);
        const float3 smbbmax = uintBitsToFloat(payload.packed1.xyz);

        SetMeshOutputsEXT(vertexCount, triangleCount);

        [[loop]]
        for (uint i = 0u; i < VERTICES_PER_MESHLET_THREAD; ++i)
        {
            const uint vertexIndex = gl_LocalInvocationID.x * VERTICES_PER_MESHLET_THREAD + i;
            const PKVertex vertex = Meshlet_Load_Vertex(vertexFirst + vertexIndex, smbbmin, smbbmax);
            float4 sv_Position;

            [[branch]]
            if (vertexIndex < vertexCount)
            {
                PK_SET_VERTEX_INSTANCE_ID(vertexIndex, instanceId)
                PK_MESHLET_FUNC_VERTEX(vertexIndex, vertex, sv_Position);
                gl_MeshVerticesEXT[vertexIndex].gl_Position = sv_Position;
            }
        }

        [[unroll]]
        for (uint i = 0u; i < TRIANGLES_PER_MESHLET_THREAD; ++i)
        {
            const uint triangleIndex = gl_LocalInvocationID.x * TRIANGLES_PER_MESHLET_THREAD + i;
            uint3 indices = uint3
            (
                PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 0u),
                PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 1u),
                PK_BUFFER_DATA(pk_Meshlet_Indices, triangleFirst * 3u + triangleIndex * 3u + 2u)
            );
            
            [[branch]]
            if (triangleIndex < triangleCount)
            {
                PK_MESHLET_FUNC_TRIANGLE(triangleIndex, indices);
                gl_PrimitiveTriangleIndicesEXT[triangleIndex] = indices;
            }
        }        
    }
#endif