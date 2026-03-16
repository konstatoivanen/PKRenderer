#pragma once

#pragma pk_program SHADER_STAGE_MESH_TASK MainMeshTask
#pragma pk_program SHADER_STAGE_MESH_ASSEMBLY MainMeshAssembly

#include "Common.glsl"
#include "Encoding.glsl"
#include "Quaternion.glsl"

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

#ifndef PK_MESHLET_USE_VERTEX_COLORS
    #define PK_MESHLET_USE_VERTEX_COLORS 0
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

uniform Buffer<uint2> pk_Meshlet_Tasklets;
uniform Buffer<uint4> pk_Meshlet_Submeshes;
uniform Buffer<uint4> pk_Meshlets;
uniform Buffer<uint4> pk_Meshlet_Vertices;
uniform Buffer<byte> pk_Meshlet_Indices;

struct PKMeshTaskPayload
{
    uint4 packed0;
    uint4 packed1;
    uint4 visible_mask; 

#if PK_MESHLET_HAS_EXTRA_PAYLOAD_DATA == 1
    PK_MESHLET_EXTRA_PAYLOAD_DATA extra;
#endif
};

// packed as uint2
struct PKTasklet
{
    uint meshlet_first;
    uint instance_id;
    uint meshlet_count;
};

// packed as 3x uint4
struct PKSubmesh
{
    float3 bbmin;
    uint meshlet_first;
    float3 bbmax;
    uint meshlet_count;
};

// first uint4 of packed meshlet
struct PKMeshletLite
{
    uint vertex_first;
    uint triangle_first;
    uint vertex_count;
    uint triangle_count;
};

// packed as 3x uint4
struct PKMeshlet
{
    uint vertex_first;
    uint triangle_first;
    float3 cone_axis;
    float cone_cutoff;
    uint vertex_count;
    uint triangle_count;
    float3 cone_apex;
    float3 center;
    float3 extents;

    float4 lod_center_error_current;
    float4 lod_center_error_parent;
};

// Based on PKAssets::PKMeshletVertex
struct PKVertex
{
    float3 position;
    float3 normal;
    #if PK_MESHLET_USE_VERTEX_COLORS
    float3 color;
    #endif
    float4 tangent;
    float2 texcoord;
};

// Unpacking functions 
PKTasklet Meshlet_Unpack_Tasklet(uint2 packed)
{
    PKTasklet t;
    t.meshlet_first = packed.x;
    t.instance_id = bitfieldExtract(packed.y, 0, 24);
    t.meshlet_count = bitfieldExtract(packed.y, 24, 8);
    return t;
}

PKSubmesh Meshlet_Unpack_Submesh(const uint4 packed0, const uint4 packed1)
{
    PKSubmesh s;
    s.bbmin = uintBitsToFloat(packed0.xyz);
    s.meshlet_first = packed0.w;
    s.bbmax = uintBitsToFloat(packed1.xyz);
    s.meshlet_count = packed1.w;
    return s;
}

PKMeshletLite Meshlet_Unpack_MeshletLite(const uint4 packed)
{
    PKMeshletLite m;
    m.vertex_first = packed.x;
    m.triangle_first = packed.y;
    m.vertex_count = bitfieldExtract(packed.w, 0, 8);
    m.triangle_count = bitfieldExtract(packed.w, 8, 8);
    return m;
}

PKMeshlet Meshlet_Unpack_Meshlet(const uint4 packed0, const uint4 packed1, const uint4 packed2)
{
    PKMeshlet m;
    m.vertex_first = packed0.x;
    m.triangle_first = packed0.y;
    m.cone_axis = unpackSnorm4x8(packed0.z).xyz;
    m.cone_cutoff = unpackSnorm4x8(packed0.z).w;
    m.vertex_count = bitfieldExtract(packed0.w, 0, 8);
    m.triangle_count = bitfieldExtract(packed0.w, 8, 8);
    m.cone_apex.x = unpackHalf2x16(packed0.w).y;
    m.cone_apex.yz = unpackHalf2x16(packed1.x);
    m.center.xy = unpackHalf2x16(packed1.y);
    m.center.z = unpackHalf2x16(packed1.z).x;
    m.extents.x = unpackHalf2x16(packed1.z).y;
    m.extents.yz = unpackHalf2x16(packed1.w);

    m.lod_center_error_current.xy = unpackHalf2x16(packed2.x);
    m.lod_center_error_current.zw = unpackHalf2x16(packed2.y);
    m.lod_center_error_parent.xy = unpackHalf2x16(packed2.z);
    m.lod_center_error_parent.zw = unpackHalf2x16(packed2.w);

    return m;
}

PKVertex Meshlet_Unpack_Vertex(const uint4 packed, const float3 sm_bbmin, const float3 sm_bbmax)
{
    PKVertex v;
    v.position.xy = unpackUnorm2x16(packed.x);
    v.position.z = unpackUnorm2x16(packed.y).x;
    v.position = lerp(sm_bbmin, sm_bbmax, v.position);
    v.texcoord = unpackHalf2x16(packed.z);

    // Decode tangent space rotation
    float4 quat;
    {
        const uint swizzle_index = bitfieldExtract(packed.w, 30, 2);
        const float4 swizzled_quat = float4
        (
            1.0f,
            (bitfieldExtract(packed.w, 0,  10) / 1023.0f) * 2.0f - 1.0f,
            (bitfieldExtract(packed.w, 10, 10) / 1023.0f) * 2.0f - 1.0f,
            (bitfieldExtract(packed.w, 20, 10) / 1023.0f) * 2.0f - 1.0f
        );

        quat = swizzled_quat;
        quat = lerp(quat, swizzled_quat.wxyz, bool(swizzle_index == 1).xxxx);
        quat = lerp(quat, swizzled_quat.zwxy, bool(swizzle_index == 2).xxxx);
        quat = lerp(quat, swizzled_quat.yzwx, bool(swizzle_index == 3).xxxx);
        quat = normalize(quat);
    }

    v.normal = Quat_MultiplyVector(quat, float3(0,0,1));
    v.tangent.xyz = Quat_MultiplyVector(quat, float3(1,0,0));
    v.tangent.w = bitfieldExtract(packed.y, 16, 1) * 2.0f - 1.0f;

    #if PK_MESHLET_USE_VERTEX_COLORS
    v.color.r = bitfieldExtract(packed.y, 17, 5) / 31.0f;
    v.color.g = bitfieldExtract(packed.y, 22, 5) / 31.0f;
    v.color.b = bitfieldExtract(packed.y, 27, 5) / 31.0f;
    #endif

    return v;
}

// Loading functions
PKTasklet Meshlet_Load_Tasklet(const uint task_index) { return Meshlet_Unpack_Tasklet(pk_Meshlet_Tasklets[task_index]); }

PKSubmesh Meshlet_Load_Submesh(const uint index) 
{ 
    return Meshlet_Unpack_Submesh(
        pk_Meshlet_Submeshes[index * 2u + 0u], 
        pk_Meshlet_Submeshes[index * 2u + 1u]); 
}

PKMeshletLite Meshlet_Load_MeshletLite(const uint index) { return Meshlet_Unpack_MeshletLite(pk_Meshlets[index * MESHLET_UINT4_STRIDE + 0u]); }

PKMeshlet Meshlet_Load_Meshlet(const uint index) 
{ 
    return Meshlet_Unpack_Meshlet(
        pk_Meshlets[index * MESHLET_UINT4_STRIDE + 0u], 
        pk_Meshlets[index * MESHLET_UINT4_STRIDE + 1u],
        pk_Meshlets[index * MESHLET_UINT4_STRIDE + 2u]); 
}

PKVertex Meshlet_Load_Vertex(const uint index, const float3 sm_bbmin, const float3 sm_bbmax) { return Meshlet_Unpack_Vertex(pk_Meshlet_Vertices[index], sm_bbmin, sm_bbmax); }

#if defined(SHADER_STAGE_MESH_TASK)

    bool Meshlet_Cull_Lod(const PKMeshlet meshlet)
    {
        #if PK_MESHLET_MAX_LOD_ONLY == 1
            return meshlet.lod_center_error_parent.w > MESHLET_MAX_ERROR;
        #else
            const float threshold = PK_MESHLET_LOD_ERROR_THRESHOLD * pk_MeshletCullParams.x;
            const float4 center_c = ObjectToClipPos(meshlet.lod_center_error_current.xyz);
            const float4 center_p = ObjectToClipPos(meshlet.lod_center_error_parent.xyz);
            float error_c = pk_Instancing_UniformScale * meshlet.lod_center_error_current.w;
            float error_p = pk_Instancing_UniformScale * meshlet.lod_center_error_parent.w;
            error_c *= inversesqrt(max(1e-6f, dot(center_c, center_c) - pow2(error_c)));
            error_p *= inversesqrt(max(1e-6f, dot(center_p, center_p) - pow2(error_p)));
            return error_p > threshold && error_c <= threshold;
        #endif 
    }
        
    bool Meshlet_Cull_Frustum_Ortho(const PKMeshlet meshlet, const float4x4 world_to_clip)
    {
        const float3 clip_center = (world_to_clip * float4(ObjectToWorldPos(meshlet.center), 1.0f)).xyz;
        const float3 clip_delta = abs(meshlet.extents.x * (world_to_clip * float4(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0], 0.0f)).xyz) + 
                                  abs(meshlet.extents.y * (world_to_clip * float4(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1], 0.0f)).xyz) + 
                                  abs(meshlet.extents.z * (world_to_clip * float4(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2], 0.0f)).xyz);
                                    
        const float3 rect_min = clip_center - clip_delta;
        const float3 rect_max = clip_center + clip_delta;
        return rect_max.z > 0.0f && rect_min.z < 1.0f && All_Greater(rect_max.xy, -1.0f.xx) && All_Less(rect_min.xy, 1.0f.xx);
    }
    
    bool Meshlet_Cull_Frustum_Perspective(const PKMeshlet meshlet, const float4x4 world_to_clip)
    {
        const float4 delta_x = (2.0f * meshlet.extents.x) * (world_to_clip * float4(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0], 0.0f));
        const float4 delta_y = (2.0f * meshlet.extents.y) * (world_to_clip * float4(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1], 0.0f));
        const float4 delta_z = (2.0f * meshlet.extents.z) * (world_to_clip * float4(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2], 0.0f));
        const float4 clip_z0 = world_to_clip * float4(ObjectToWorldPos(meshlet.center - meshlet.extents), 1.0f);
        const float4 clip_z1 = clip_z0 + delta_z;

        float4 pmin = 1.0f.xxxx;

        for (uint i = 0; i < 4; ++i)
        {
            const float4 clip_0 = clip_z0 + delta_x * (i & 0x1) + delta_y * (i / 2);
            const float4 clip_1 = clip_z1 + delta_x * (i & 0x1) + delta_y * (i / 2);
            pmin = min(pmin, float4(clip_0.xy, -clip_0.xy) - clip_0.w);
            pmin = min(pmin, float4(clip_1.xy, -clip_1.xy) - clip_1.w);
        }
    
        return All_Less(pmin, 0.0f.xxxx);
    }
    
    bool Meshlet_Cull_Sphere(const PKMeshlet meshlet, float3 center, float radius)
    {
        center -= ObjectToWorldPos(meshlet.center);
        const float3 vx = 2.0f * meshlet.extents.x * float3(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0]);
        const float3 vy = 2.0f * meshlet.extents.y * float3(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1]);
        const float3 vz = 2.0f * meshlet.extents.z * float3(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2]);
        const float3 base = -0.5f * (vx + vy + vz);
        float3 extents = 0.0f.xxx;

        for (uint i = 0; i < 4; ++i)
        {
            const float3 pmin = base + vz * 0 + vx * (i & 0x1) + vy * (i / 2);
            const float3 pmax = base + vz * 1 + vx * (i & 0x1) + vy * (i / 2);
            extents = max(max(extents, pmin), pmax);
        }

        float3 d = abs(center) - extents;
        float r = radius - cmax(min(d, float3(0.0f)));
        d = max(d, float3(0.0f));
        return dot(d, d) <= r * r;
    }
    
    bool Meshlet_Cull_Cone(const PKMeshlet meshlet, float3 cull_origin)
    {
        const float3 cone_axis = SafeNormalize(ObjectToWorldVec(meshlet.cone_axis));
        const float3 cone_apex = ObjectToWorldPos(meshlet.cone_apex);
        const float3 cone_view = normalize(cone_apex - cull_origin);
        return dot(cone_view, cone_axis) < meshlet.cone_cutoff;
    }
    
    bool Meshlet_Cull_Cone_Directional(const PKMeshlet meshlet, float3 cull_direction)
    {
        const float3 cone_axis = SafeNormalize(ObjectToWorldVec(meshlet.cone_axis));
        return dot(cull_direction, cone_axis) < meshlet.cone_cutoff;
    }
    
    #if defined(PK_MESHLET_SAMPLE_VIEW_DEPTH_MIP)
    bool Meshlet_Cull_Depth(const bool is_visible, const bool is_clipped_near, const float3 rect_min, const float3 rect_max, const uint2 resolution)
    {
        [[branch]]
        if (is_visible && !is_clipped_near)
        {
            const float4 rect_uv = saturate(float4(rect_min.xy, rect_max.xy) * 0.5f + 0.5f);
            const float2 rect_size = (rect_uv.zw - rect_uv.xy) * resolution;
            const float level = ceil(log2(max(rect_size.x, rect_size.y)));
            const float depth_00 = PK_MESHLET_SAMPLE_VIEW_DEPTH_MIP(rect_uv.xy, level);
            const float depth_01 = PK_MESHLET_SAMPLE_VIEW_DEPTH_MIP(rect_uv.xw, level);
            const float depth_11 = PK_MESHLET_SAMPLE_VIEW_DEPTH_MIP(rect_uv.zw, level);
            const float depth_10 = PK_MESHLET_SAMPLE_VIEW_DEPTH_MIP(rect_uv.zy, level);
            const float depth_max = max(max(depth_00, depth_01), max(depth_11, depth_10));
            return ViewDepth(rect_max.z) <= depth_max;
        }
    
        return is_visible;
    }

    bool Meshlet_Cull_Depth_Ortho(const PKMeshlet meshlet, const float4x4 world_to_clip, const uint2 resolution)
    {
        const float3 clip_center = (world_to_clip * float4(ObjectToWorldPos(meshlet.center), 1.0f)).xyz;
        const float3 clip_delta = abs(meshlet.extents.x * (world_to_clip * float4(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0], 0.0f)).xyz) + 
                                  abs(meshlet.extents.y * (world_to_clip * float4(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1], 0.0f)).xyz) + 
                                  abs(meshlet.extents.z * (world_to_clip * float4(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2], 0.0f)).xyz);
        
        const float3 rect_min = clip_center - clip_delta;
        const float3 rect_max = clip_center + clip_delta;
        const bool is_clipped_far = rect_min.z < 0.0f;
        const bool is_clipped_near = rect_max.z > 1.0f;
        const bool is_visible = rect_max.z > 0.0f && rect_min.z < 1.0f && All_Greater(rect_max.xy, -1.0f.xx) && All_Less(rect_min.xy, 1.0f.xx);
        return Meshlet_Cull_Depth(is_visible, is_clipped_near, rect_min, rect_max, resolution);
    }

    bool Meshlet_Cull_Depth_Perspective(const PKMeshlet meshlet, const float4x4 world_to_clip, const float4x4 view_to_clip, const uint2 resolution)
    {
        const float4 delta_x = (2.0f * meshlet.extents.x) * (world_to_clip * float4(pk_ObjectToWorld[0][0], pk_ObjectToWorld[1][0], pk_ObjectToWorld[2][0], 0.0f));
        const float4 delta_y = (2.0f * meshlet.extents.y) * (world_to_clip * float4(pk_ObjectToWorld[0][1], pk_ObjectToWorld[1][1], pk_ObjectToWorld[2][1], 0.0f));
        const float4 delta_z = (2.0f * meshlet.extents.z) * (world_to_clip * float4(pk_ObjectToWorld[0][2], pk_ObjectToWorld[1][2], pk_ObjectToWorld[2][2], 0.0f));
        const float4 clip_z0 = world_to_clip * float4(ObjectToWorldPos(meshlet.center - meshlet.extents), 1.0f);
        const float4 clip_z1 = clip_z0 + delta_z;
    
        float min_w = +1e+38f;
        float max_w = -1e+38f;
        float4 pmin = 1.0f.xxxx;
        float3 rect_min = float3(+1.0f, +1.0f, +1.0f);
        float3 rect_max = float3(-1.0f, -1.0f, -1.0f);
        
        [[loop]]
        for (uint i = 0; i < 4; ++i)
        {
            const float4 clip_0 = clip_z0 + delta_x * (i & 0x1) + delta_y * (i / 2);
            const float4 clip_1 = clip_z1 + delta_x * (i & 0x1) + delta_y * (i / 2);
            const float2 clip_0_xy = clip_0.xy / clip_0.w;
            const float2 clip_1_xy = clip_1.xy / clip_1.w;
            pmin = min(pmin, float4(clip_0.xy, -clip_0.xy) - clip_0.w);
            pmin = min(pmin, float4(clip_1.xy, -clip_1.xy) - clip_1.w);
            min_w = min(min(min_w, clip_0.w), clip_1.w);
            max_w = max(max(max_w, clip_0.w), clip_1.w); 
            rect_min.xy = min(min(rect_min.xy, clip_0_xy), clip_1_xy); 
            rect_max.xy = max(max(rect_max.xy, clip_0_xy), clip_1_xy);
        }
    
        const float min_z = max_w * view_to_clip[2][2] + view_to_clip[3][2]; // Flipped from 3,2
        const float max_z = min_w * view_to_clip[2][2] + view_to_clip[3][2]; // Flipped from 3,2
    
        if (min_w <= 0.0f && max_w > 0.0f)
        {
            rect_min = float3(-1.0f, -1.0f, -1.0f);
            rect_max = float3(+1.0f, +1.0f, +1.0f);
        }
        else
        {
            rect_min.z = min_z / max_w;
            rect_max.z = max_z / min_w;
        }
        
        const bool is_clipped_near = min_w <= max_z;
        const bool is_clipped_far = 0.0f >= min_z;
        const bool is_visible = max_w > min_z && 0.0f < max_z && All_Less(pmin, 0.0f.xxxx);
        return Meshlet_Cull_Depth(is_visible, is_clipped_near, rect_min, rect_max, resolution);
    }

    #else
    bool Meshlet_Cull_Hierarchical_Depth_Ortho(const PKMeshlet meshlet, const float4x4 world_to_clip, const uint2 resolution) { return false; }
    bool Meshlet_Cull_Hierarchical_Depth_Perspective(const PKMeshlet meshlet, const float4x4 world_to_clip, const float4x4 view_to_clip, const uint2 resolution) { return false; }
    #endif

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

    uniform uint pk_Meshlet_DispatchOffset;
    taskPayloadSharedEXT PKMeshTaskPayload payload;

    layout(local_size_x = MAX_MESHLETS_PER_TASK) in;
    void MainMeshTask()
    {
        uint meshlet_count;
        
        [[branch]]
        if (subgroupElect())
        {
            const uint task_index = pk_Meshlet_DispatchOffset + gl_WorkGroupID.x;
            const PKTasklet tasklet = Meshlet_Load_Tasklet(task_index);
            PK_INSTANCING_ASSIGN_LOCALS(tasklet.instance_id);
            const PKSubmesh submesh = Meshlet_Load_Submesh(pk_Instancing_Submesh);
            payload.packed0.xyz = floatBitsToUint(submesh.bbmin);
            payload.packed1.xyz = floatBitsToUint(submesh.bbmax);
            payload.packed0.w = tasklet.meshlet_first;
            payload.packed1.w = tasklet.instance_id;
            meshlet_count = tasklet.meshlet_count;
            PK_MESHLET_FUNC_TASKLET(payload);
        }
    
        subgroupBarrier();

        PK_INSTANCING_BROADCAST_LOCALS_MANUAL();

        const uint meshlet_first = payload.packed0.w;
        meshlet_count = subgroupBroadcastFirst(meshlet_count);
    
        const uint meshlet_index_local = gl_LocalInvocationID.x;
        const uint meshlet_index = meshlet_first + meshlet_index_local;
        const PKMeshlet meshlet = Meshlet_Load_Meshlet(meshlet_index);
        
        bool is_visible = true;
        is_visible = is_visible && meshlet_index_local < meshlet_count;
        is_visible = is_visible && Meshlet_Cull_Lod(meshlet);
        is_visible = is_visible && PK_MESHLET_FUNC_CULL(meshlet);

        const uint4 visible_mask = subgroupBallot(is_visible);
        const uint visible_count = subgroupBallotBitCount(visible_mask);
    
        payload.visible_mask = visible_mask;

        EmitMeshTasksEXT(visible_count, 1, 1);
    }

#elif defined(SHADER_STAGE_MESH_ASSEMBLY)

    void PK_MESHLET_FUNC_VERTEX(uint vertex_index, PKVertex vertex, inout float4 sv_Position);

    #if PK_MESHLET_USE_FUNC_TRIANGLE == 1
    void PK_MESHLET_FUNC_TRIANGLE(uint triangle_index, inout uint3 indices);
    #else
    void PK_MESHLET_FUNC_TRIANGLE(uint triangle_index, inout uint3 indices) {}
    #endif

    taskPayloadSharedEXT PKMeshTaskPayload payload;

    layout(local_size_x = MESHLET_LOCAL_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;
    layout(triangles, max_vertices = MAX_VERTICES_PER_MESHLET, max_primitives = MAX_TRIANGLES_PER_MESHLET) out;
    void MainMeshAssembly()
    {
        const uint meshlet_first = payload.packed0.w;
        const uint instance_id = payload.packed1.w;

        uint delta_index = subgroupBallotExclusiveBitCount(payload.visible_mask);
        delta_index = lerp(0u, gl_LocalInvocationID.x, delta_index == gl_WorkGroupID.x);
        delta_index = subgroupMax(delta_index);

        const uint meshlet_index = meshlet_first + delta_index;
        const PKMeshletLite meshlet = Meshlet_Load_MeshletLite(meshlet_index);
        
        PK_INSTANCING_ASSIGN_LOCALS(instance_id);

        const uint vertex_first = meshlet.vertex_first;
        const uint vertex_count = meshlet.vertex_count;
        const uint triangle_first = meshlet.triangle_first;
        const uint triangle_count = meshlet.triangle_count;
        const float3 smb_bmin = uintBitsToFloat(payload.packed0.xyz);
        const float3 smb_bmax = uintBitsToFloat(payload.packed1.xyz);

        SetMeshOutputsEXT(vertex_count, triangle_count);

        [[loop]]
        for (uint i = 0u; i < VERTICES_PER_MESHLET_THREAD; ++i)
        {
            const uint vertex_index = gl_LocalInvocationID.x * VERTICES_PER_MESHLET_THREAD + i;
            const PKVertex vertex = Meshlet_Load_Vertex(vertex_first + vertex_index, smb_bmin, smb_bmax);
            float4 sv_Position;

            [[branch]]
            if (vertex_index < vertex_count)
            {
                PK_INSTANCING_ASSIGN_VERTEX_INSTANCE_ID(vertex_index, instance_id)
                PK_MESHLET_FUNC_VERTEX(vertex_index, vertex, sv_Position);
                gl_MeshVerticesEXT[vertex_index].gl_Position = sv_Position;
            }
        }

        [[unroll]]
        for (uint i = 0u; i < TRIANGLES_PER_MESHLET_THREAD; ++i)
        {
            const uint triangle_index = gl_LocalInvocationID.x * TRIANGLES_PER_MESHLET_THREAD + i;
            uint3 indices = uint3
            (
                pk_Meshlet_Indices[triangle_first * 3u + triangle_index * 3u + 0u],
                pk_Meshlet_Indices[triangle_first * 3u + triangle_index * 3u + 1u],
                pk_Meshlet_Indices[triangle_first * 3u + triangle_index * 3u + 2u]
            );
            
            [[branch]]
            if (triangle_index < triangle_count)
            {
                PK_MESHLET_FUNC_TRIANGLE(triangle_index, indices);
                gl_PrimitiveTriangleIndicesEXT[triangle_index] = indices;
            }
        }        
    }
#endif
