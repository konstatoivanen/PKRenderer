#ZTest GEqual
#ZWrite False
#LogVerbose
#include includes/Common.glsl
#include includes/Encoding.glsl
#include includes/SceneEnv.glsl
#include includes/VolumeFog.glsl

#pragma PROGRAM_MESH_TASK

void main()
{
    EmitMeshTasksEXT(3, 1, 1);
}

#pragma PROGRAM_MESH_ASSEMBLY

float4 PK_BLIT_VERTEX_POSITIONS[3] =
{
    float4(-1.0,  1.0, 0.0, 1.0),
    float4(-1.0, -3.0, 0.0, 1.0),
    float4(3.0,  1.0, 0.0, 1.0),
};

out float3 vs_TEXCOORD[];

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 3, max_primitives = 1) out;
void main()
{
	//uint iid = gl_LocalInvocationID.x;

	SetMeshOutputsEXT(3, 1);
	
    gl_MeshVerticesEXT[0].gl_Position = PK_BLIT_VERTEX_POSITIONS[0];
	gl_MeshVerticesEXT[1].gl_Position = PK_BLIT_VERTEX_POSITIONS[1];
	gl_MeshVerticesEXT[2].gl_Position = PK_BLIT_VERTEX_POSITIONS[2];

    vs_TEXCOORD[0] = float4(PK_BLIT_VERTEX_POSITIONS[0].xy * pk_ClipParamsInv.xy, 1.0f, 0.0f) * pk_ViewToWorld;
    vs_TEXCOORD[1] = float4(PK_BLIT_VERTEX_POSITIONS[1].xy * pk_ClipParamsInv.xy, 1.0f, 0.0f) * pk_ViewToWorld;
    vs_TEXCOORD[2] = float4(PK_BLIT_VERTEX_POSITIONS[2].xy * pk_ClipParamsInv.xy, 1.0f, 0.0f) * pk_ViewToWorld;
	
    gl_PrimitiveTriangleIndicesEXT[gl_LocalInvocationIndex] = uint3(0, 1, 2);
}

#pragma PROGRAM_FRAGMENT

in float3 vs_TEXCOORD0;
layout(location = 0) out float4 SV_Target0;

void main()
{
    const float3 viewdir = normalize(vs_TEXCOORD0);
    const float2 octaUV = OctaUV(viewdir);
    float3 color = SampleEnvironment(octaUV, 0.0f);
    VFog_ApplySky(viewdir, color);
    SV_Target0 = float4(color, 1.0f);
}