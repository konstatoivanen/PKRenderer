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

PK_DECLARE_BUFFER(uint2, pk_Tasklets, PK_SET_SHADER);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Submeshes, PK_SET_SHADER);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlets, PK_SET_SHADER);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(uint4, pk_Meshlet_Vertices, PK_SET_SHADER);
PK_DECLARE_RESTRICTED_READONLY_BUFFER(byte, pk_Meshlet_Indices, PK_SET_SHADER);

#define MP_MATERIAL instance.x
#define MP_TRANSFORM instance.y
#define MP_FIRSTMESHLET instance.z
#define MP_USERDATA instance.w

struct PKMeshTaskPayload
{
    uint4 instance;
    byte deltaIDs[MAX_MESHLETS_PER_TASK];
};

// packed as uint2
struct PKTasklet
{
    uint firstMeshlet;
    uint instanceId;
    uint meshletCount;
};

// packed as 3x uint4
struct PKSubmesh
{
    uint firstMeshlet;
    uint firstVertex;
    uint firstTriangle;
    uint meshletCount;
    uint vertexCount;
    uint triangleCount;
    float3 bbmin;
    float3 bbmax;
};

// packed as 2x uint4
struct PKMeshlet
{
    uint firstVertex;
    uint firstTriangle;
    uint vertexCount;   // ushort
    uint triangleCount; // ushort
    float2 coneAxis;    // half
    float3 center;      // half
    float radius;       // half
    float3 coneApex;    // half
    float coneCutoff;   // half
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
    t.firstMeshlet = packed.x;
    t.instanceId = bitfieldExtract(packed.y, 0, 24);
    t.meshletCount = bitfieldExtract(packed.y, 24, 8);
    return t;
}

uint2 Meshlet_Pack_Tasklet(PKTasklet t)
{
    uint2 p;
    p.x = t.firstMeshlet;
    p.y = bitfieldInsert(p.y, t.instanceId, 0, 24);
    p.y = bitfieldInsert(p.y, t.meshletCount, 24, 8);
    return p;
}

PKSubmesh Meshlet_Unpack_Submesh(uint4 packed0, uint4 packed1, uint4 packed2)
{
    PKSubmesh s;
    s.firstMeshlet = packed0.x;
    s.firstVertex = packed0.y;
    s.firstTriangle = packed0.z;
    s.meshletCount = packed0.w;
    s.vertexCount = packed1.x;
    s.triangleCount = packed1.y;
    s.bbmin.xy = uintBitsToFloat(packed1.zw);
    s.bbmin.z = uintBitsToFloat(packed2.x);
    s.bbmax = uintBitsToFloat(packed2.yzw);
    return s;
}

PKMeshlet Meshlet_Unpack_Meshlet(uint4 packed0, uint4 packed1)
{
    PKMeshlet m;
    m.firstVertex = packed0.x;
    m.firstTriangle = packed0.y;
    m.vertexCount = bitfieldExtract(packed0.z, 0, 16);   // ushort
    m.triangleCount = bitfieldExtract(packed0.z, 16, 16); // ushort
    m.coneAxis = unpackUnorm2x16(packed0.w);
    
    float4 v0 = unpackHalf4x16(packed1.xy);
    m.center = v0.xyz;
    m.radius = v0.w;

    float4 v1 = unpackHalf4x16(packed1.zw);
    m.coneApex = v1.xyz;
    m.coneCutoff = v1.w;

    return m;
}

PKVertex Meshlet_Unpack_Vertex(uint4 packed, float3 center, float radius)
{
    PKVertex v;
    v.position.x = (bitfieldExtract(packed.x, 0, 11) / 2047.0f) * 2.0f - 1.0f;
    v.position.y = (bitfieldExtract(packed.x, 11, 11) / 2047.0f) * 2.0f - 1.0f;
    v.position.z = (bitfieldExtract(packed.x, 22, 10) / 1023.0f) * 2.0f - 1.0f;
    v.position *= radius;
    v.position += center;
    v.texcoord = unpackHalf2x16(packed.y);
    v.normal = OctaDecode(unpackUnorm2x16(packed.z));

    float2 tangentOctaUV;
    tangentOctaUV.x = bitfieldExtract(packed.w, 0, 15) / 32767.0f;
    tangentOctaUV.y = bitfieldExtract(packed.w, 15, 15) / 32767.0f;
    float tangentSign = bitfieldExtract(packed.w, 30, 3) == 0 ? -1.0f : 1.0f;
    v.tangent = float4(OctaDecode(tangentOctaUV), tangentSign);

    return v;
}

PKSubmesh Meshlet_Load_Submesh(uint index) 
{ 
    uint4 packed0 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 3u + 0u);
    uint4 packed1 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 3u + 1u);
    uint4 packed2 = PK_BUFFER_DATA(pk_Meshlet_Submeshes, index * 3u + 2u);
    return Meshlet_Unpack_Submesh(packed0, packed1, packed2);
}

PKMeshlet Meshlet_Load_Meshlet(uint index)
{
    uint4 packed0 = PK_BUFFER_DATA(pk_Meshlets, index * 2u + 0u);
    uint4 packed1 = PK_BUFFER_DATA(pk_Meshlets, index * 2u + 1u);
    return Meshlet_Unpack_Meshlet(packed0, packed1);
}

PKVertex Meshlet_Load_Vertex(uint index, float3 center, float radius) 
{
    return Meshlet_Unpack_Vertex(PK_BUFFER_DATA(pk_Meshlet_Vertices, index), center, radius);
}

PKTasklet Meshlet_Load_Tasklet(uint taskIndex)
{
    return Meshlet_Unpack_Tasklet(PK_BUFFER_DATA(pk_Tasklets, taskIndex));
}

void Meshlet_Store_Tasklet(uint taskIndex, const PKTasklet t)
{
    PK_BUFFER_DATA(pk_Tasklets, taskIndex) = Meshlet_Pack_Tasklet(t);
}
