
#pragma pk_cull Back
#pragma pk_ztest Greater
#pragma pk_zwrite True
#pragma pk_enable_instancing
#pragma pk_disable_fragment_instancing
#pragma pk_multi_compile PK_LIGHT_PASS_DIRECTIONAL PK_LIGHT_PASS_SPOT PK_LIGHT_PASS_POINT
#pragma pk_program Fragment MainFs

struct LightPayload
{
    float4x4 light_matrix;
    float3 light_position;
    float light_znear;
    float light_radius;
    uint layer;
};

// Higher error threshold for shadows as accuracy is not that important.
#define PK_MESHLET_LOD_SCALE 6.0f
#define PK_MESHLET_USE_FUNC_CULL 1
#define PK_MESHLET_USE_FUNC_TRIANGLE 1
#define PK_MESHLET_USE_FUNC_TASKLET 1
#define PK_MESHLET_EXTRA_PAYLOAD_DATA LightPayload

struct ShadowVertexUserData
{
#if defined(PK_LIGHT_PASS_DIRECTIONAL)
    float vs_DEPTH : TEXCOORD0;
#else
    float3 vs_DEPTH : TEXCOORD0;
#endif
};

#define PK_MESHLET_VERTEX_USER_DATA ShadowVertexUserData

#include "includes/LightResources.glsl"
#include "includes/Meshlets.glsl"

// As opposed to default order y axis is flipped here to avoid having to switch winding order for cube depth rendering
static const float3x3 PK_CUBE_FACE_MATRICES[6] =
{
    float3x3(+0,+0,-1, +0,+1,+0, +1,+0,+0), // Right
    float3x3(+0,+0,+1, +0,+1,+0, -1,+0,+0), // Left
    float3x3(+1,+0,+0, +0,+0,+1, +0,-1,+0), // Down
    float3x3(+1,+0,+0, +0,+0,-1, +0,+1,+0), // Up
    float3x3(+1,+0,+0, +0,+1,+0, +0,+0,+1), // Front
    float3x3(-1,+0,+0, +0,+1,+0, +0,+0,-1), // Back
};

float4 GetCubeClipPos(float3 viewvec, float near, float far, uint face_index)
{
    const float3 view_pos = viewvec * PK_CUBE_FACE_MATRICES[face_index];
    const float m22 = -near / (far - near);
    const float m32 = (near * far) / (far - near);
    return float4(view_pos.xy, m22 * view_pos.z + m32, view_pos.z);
}

void PK_MESHLET_FUNC_TASKLET(inout PKMeshTaskPayload payload)
{
    const uint light_index = bitfieldExtract(pk_Instancing_Userdata, 0, 16);
    const uint layer = bitfieldExtract(pk_Instancing_Userdata, 16, 16);
    const SceneLight light = Lights_LoadLight(light_index);

    payload.extra.light_position = light.position;
    payload.extra.light_znear = light.near_clip;
    payload.extra.light_radius = light.radius;
    payload.extra.layer = layer;

    float4x4 light_matrix;

    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        light_matrix = pk_LightMatrices[light.index_shadow + layer];
    #elif defined(PK_LIGHT_PASS_SPOT)
        light_matrix = pk_LightMatrices[light.index_shadow];
    #endif

    payload.extra.light_matrix = light_matrix;
}

bool PK_MESHLET_FUNC_CULL(PKMeshTaskPayload payload, const PKMeshlet meshlet)
{
    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        return Meshlet_Cull_Cone_Directional(meshlet, payload.extra.light_position) && Meshlet_Cull_Frustum_Ortho(meshlet, payload.extra.light_matrix);
    #elif defined(PK_LIGHT_PASS_SPOT)
        return Meshlet_Cull_Cone(meshlet, payload.extra.light_position) && Meshlet_Cull_Frustum_Perspective(meshlet, payload.extra.light_matrix);
    #else
        return Meshlet_Cull_Cone(meshlet, payload.extra.light_position) && Meshlet_Cull_Sphere(meshlet, payload.extra.light_position, payload.extra.light_radius);
    #endif
}

void PK_MESHLET_FUNC_TRIANGLE(uint triangle_index, PKMeshTaskPayload payload, inout uint3 indices, inout PKMeshPrimitiveOut pout)
{
    pout.sv_Layer = payload.extra.layer;
}

void PK_MESHLET_FUNC_VERTEX(uint vertex_index, PKMeshTaskPayload payload, PKVertex vertex, inout PKMeshletVertexVaryings outVaryings)
{
    const float3 world_pos = ObjectToWorldPos(vertex.position);

    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        outVaryings.sv_Position = payload.extra.light_matrix * float4(world_pos, 1.0f);
        // Depth test uses reverse z for precision reasons. revert range for actual distance.
        outVaryings.userdata.vs_DEPTH = dot(payload.extra.light_position, world_pos) + payload.extra.light_radius;
    #elif defined(PK_LIGHT_PASS_SPOT)
        outVaryings.sv_Position = payload.extra.light_matrix * float4(world_pos, 1.0f);
        outVaryings.userdata.vs_DEPTH = world_pos - payload.extra.light_position;
    #elif defined(PK_LIGHT_PASS_POINT)
        const float3 vs_depth = world_pos - payload.extra.light_position;
        outVaryings.sv_Position = GetCubeClipPos(vs_depth, payload.extra.light_znear, payload.extra.light_radius, payload.extra.layer % 6);
        outVaryings.userdata.vs_DEPTH = vs_depth;
    #endif
}

[earlydepthstencil]
float MainFs(PKMeshletVertexVaryings vout) : SV_Target0
{
    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        return vout.userdata.vs_DEPTH;
    #else
        return length(vout.userdata.vs_DEPTH.xyz);
    #endif
}
