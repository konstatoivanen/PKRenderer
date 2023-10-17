#version 460
#Cull Back
#ZTest GEqual
#ZWrite True
#EnableInstancing

#include includes/LightResources.glsl

#pragma PROGRAM_VERTEX

// As opposed to default order y axis is flipped here to avoid having to switch winding order for cube depth rendering
const float3x3 PK_CUBE_FACE_MATRICES[6] =
{
	// Right
	float3x3( 0,  0, -1, 
			 0,  1,  0,
			 1,  0,  0), 

	// Left
	float3x3( 0,  0,  1, 
			 0,  1,  0,
			-1,  0,  0), 

	// Down
	float3x3( 1,  0,  0, 
			 0,  0,  1,
			 0, -1,  0), 

	// Up
	float3x3( 1,  0,  0, 
			 0,  0, -1,
			 0,  1,  0), 

	// Front
	float3x3( 1,  0,  0, 
			  0,  1,  0,
			  0,  0,  1), 

	// Back
	float3x3(-1,  0,  0, 
			 0,  1,  0,
			 0,  0, -1), 
};

float4 GetCubeClipPos(float3 viewvec, float radius, uint faceIndex)
{
	const float3 vpos = viewvec * PK_CUBE_FACE_MATRICES[faceIndex];
    const float near = 0.1f;
    const float far = radius;
    const float m22 = -near / (far - near);
    const float m32 = (near * far) / (far - near);
    return float4(vpos.xy, m22 * vpos.z + m32, vpos.z);
}

in float3 in_POSITION;
out float3 vs_DEPTH;

void main()
{
    const uint lightIndex = bitfieldExtract(pk_Instancing_Userdata, 0, 16);
    const uint layer = bitfieldExtract(pk_Instancing_Userdata, 16, 16);
    const LightPacked light = Lights_LoadPacked(lightIndex);
    const float3 wpos = ObjectToWorldPos(in_POSITION);
    const uint projectionIndex = light.LIGHT_PROJECTION;

    float4 vs_pos = 0.0f.xxxx;
    float3 vs_depth = 0.0f.xxx;

    [[branch]]
    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_POINT:
        {
            vs_depth = wpos - light.LIGHT_POS;
            vs_pos = GetCubeClipPos(vs_depth, light.LIGHT_RADIUS, layer % 6);
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            float4x4 lightmatrix = PK_BUFFER_DATA(pk_LightMatrices, projectionIndex);
            vs_pos = mul(lightmatrix, float4(wpos, 1.0f));
            vs_depth.xyz = wpos - light.LIGHT_POS;
        }
        break;
        case LIGHT_TYPE_DIRECTIONAL:
        {
            float4x4 lightmatrix = PK_BUFFER_DATA(pk_LightMatrices, projectionIndex + layer);
            vs_pos = mul(lightmatrix, float4(wpos, 1.0f));

            // Depth test uses reverse z for precision reasons. revert range for actual distance.
            vs_depth.x = (1.0f - (vs_pos.z / vs_pos.w)) * light.LIGHT_RADIUS;
        }
        break;
    }

    gl_Layer = int(layer);
    gl_Position = vs_pos;
    vs_DEPTH = vs_depth;
}

#pragma PROGRAM_FRAGMENT

in float3 vs_DEPTH;

layout(early_fragment_tests) in;
layout(location = 0) out float SV_Target0;
void main()
{
    // Store linear distance into shadowmaps.
    // Point lights use an octahedral layout which doesnt mesh well with clip space depth.
    SV_Target0 = length(vs_DEPTH.xyz);
}