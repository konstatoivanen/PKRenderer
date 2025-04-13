
#pragma pk_cull Back
#pragma pk_ztest GEqual
#pragma pk_zwrite True
#pragma pk_enable_instancing
#pragma pk_disable_fragment_instancing
#pragma pk_multi_compile PK_LIGHT_PASS_DIRECTIONAL PK_LIGHT_PASS_SPOT PK_LIGHT_PASS_POINT
#pragma pk_program SHADER_STAGE_FRAGMENT MainFs

struct LightPayload
{
    float4x4 light_matrix;
    float3 light_position;
    float light_radius;
    uint layer;
};

// Higher error threshold for shadows as accuracy is not that important.
#define PK_MESHLET_LOD_ERROR_THRESHOLD 0.5f
#define PK_MESHLET_USE_FUNC_CULL 1
#define PK_MESHLET_USE_FUNC_TRIANGLE 1
#define PK_MESHLET_USE_FUNC_TASKLET 1
#define PK_MESHLET_HAS_EXTRA_PAYLOAD_DATA 1
#define PK_MESHLET_EXTRA_PAYLOAD_DATA LightPayload
#include "includes/LightResources.glsl"
#include "includes/Meshlets.glsl"

#if defined(PK_LIGHT_PASS_DIRECTIONAL)
    PK_DECLARE_VS_ATTRIB(float vs_DEPTH);
#else
    PK_DECLARE_VS_ATTRIB(float3 vs_DEPTH);
#endif

// As opposed to default order y axis is flipped here to avoid having to switch winding order for cube depth rendering
const float3x3 PK_CUBE_FACE_MATRICES[6] =
{
    // Right
    float3x3(+0,+0,-1,
             +0,+1,+0,
             +1,+0,+0),

    // Left
    float3x3(+0,+0,+1,
             +0,+1,+0,
             -1,+0,+0),

    // Down
    float3x3(+1,+0,+0,
             +0,+0,+1,
             +0,-1,+0),

    // Up
    float3x3(+1,+0,+0,
             +0,+0,-1,
             +0,+1,+0),

    // Front
    float3x3(+1,+0,+0,
             +0,+1,+0,
             +0,+0,+1),

    // Back
    float3x3(-1,+0,+0,
             +0,+1,+0,
             +0,+0,-1),
};

float4 GetCubeClipPos(float3 viewvec, float radius, uint face_index)
{
    const float3 view_pos = viewvec * PK_CUBE_FACE_MATRICES[face_index];
    const float near = 0.1f;
    const float far = radius;
    const float m22 = -near / (far - near);
    const float m32 = (near * far) / (far - near);
    return float4(view_pos.xy, m22 * view_pos.z + m32, view_pos.z);
}

[[pk_restrict STAGE_MESH_TASK]] 
void PK_MESHLET_FUNC_TASKLET(inout PKMeshTaskPayload payload)
{
    const uint light_index = bitfieldExtract(pk_Instancing_Userdata, 0, 16);
    const LightPacked light = Lights_LoadPacked(light_index);
    const uint matrix_index = light.LIGHT_MATRIX;
    const uint layer = bitfieldExtract(pk_Instancing_Userdata, 16, 16);

    payload.extra.light_position = light.LIGHT_POS;
    payload.extra.light_radius = light.LIGHT_RADIUS;
    payload.extra.layer = layer;

    float4x4 light_matrix;

    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        light_matrix = PK_BUFFER_DATA(pk_LightMatrices, matrix_index + layer);
    #elif defined(PK_LIGHT_PASS_SPOT)
        light_matrix = PK_BUFFER_DATA(pk_LightMatrices, matrix_index);
    #endif

    payload.extra.light_matrix = light_matrix;
}

[[pk_restrict STAGE_MESH_TASK]] 
bool PK_MESHLET_FUNC_CULL(const PKMeshlet meshlet)
{
    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        return Meshlet_Cone_Cull_Directional(meshlet, payload.extra.light_position) && Meshlet_Frustum_Ortho_Cull(meshlet, payload.extra.light_matrix);
    #elif defined(PK_LIGHT_PASS_SPOT)
        return Meshlet_Cone_Cull(meshlet, payload.extra.light_position) && Meshlet_Frustum_Perspective_Cull(meshlet, payload.extra.light_matrix);
    #else
        return Meshlet_Cone_Cull(meshlet, payload.extra.light_position) && Meshlet_Sphere_Cull(meshlet, payload.extra.light_position, payload.extra.light_radius);
    #endif
}

[[pk_restrict STAGE_MESH_ASSEMBLY]] 
void PK_MESHLET_FUNC_TRIANGLE(uint triangle_index, inout uint3 indices)
{
    gl_MeshPrimitivesEXT[triangle_index].gl_Layer = int(payload.extra.layer);
}

[[pk_restrict STAGE_MESH_ASSEMBLY]] 
void PK_MESHLET_FUNC_VERTEX(uint vertex_index, PKVertex vertex, inout float4 sv_Position)
{
    const float3 world_pos = ObjectToWorldPos(vertex.position);

    #if defined(PK_LIGHT_PASS_DIRECTIONAL)

        sv_Position = payload.extra.light_matrix * float4(world_pos, 1.0f);
        // Depth test uses reverse z for precision reasons. revert range for actual distance.
        vs_DEPTH[vertex_index] = dot(payload.extra.light_position, world_pos) + payload.extra.light_radius;

    #elif defined(PK_LIGHT_PASS_SPOT)

        sv_Position = payload.extra.light_matrix * float4(world_pos, 1.0f);
        vs_DEPTH[vertex_index] = world_pos - payload.extra.light_position;

    #elif defined(PK_LIGHT_PASS_POINT)

        const float3 vs_depth = world_pos - payload.extra.light_position;
        sv_Position = GetCubeClipPos(vs_depth, payload.extra.light_radius, payload.extra.layer % 6);
        vs_DEPTH[vertex_index] = vs_depth;

    #endif
}

[[pk_restrict STAGE_FRAGMENT]] out float SV_Target0;
[[pk_restrict STAGE_FRAGMENT]] layout(early_fragment_tests) in;

void MainFs()
{
    // Store linear distance into shadowmaps.
    // Point lights use an octahedral layout which doesnt mesh well with clip space depth.
    #if defined(PK_LIGHT_PASS_DIRECTIONAL)
        SV_Target0 = vs_DEPTH;
    #else
        SV_Target0 = length(vs_DEPTH.xyz);
    #endif
}
