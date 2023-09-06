#version 460
#Cull Back
#ZTest LEqual
#ZWrite True
#EnableInstancing

#include includes/Common.glsl
#include includes/SharedLights.glsl

#pragma PROGRAM_VERTEX

#define SHADOW_NEAR_BIAS 0.1f

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
	float3 vpos = viewvec * PK_CUBE_FACE_MATRICES[faceIndex];
    float4 clip = float4(vpos.xy, 1.020202f * vpos.z - radius * 0.020202f, vpos.z);
    clip.z = (clip.z + clip.w) / 2.0f;
    return clip;
}

in float3 in_POSITION;
out float4 vs_DEPTH;

void main()
{
    const uint lightIndex = bitfieldExtract(pk_Instancing_Userdata, 0, 16);
    const uint layer = bitfieldExtract(pk_Instancing_Userdata, 16, 16);
    const LightPacked light = PK_BUFFER_DATA(pk_Lights, lightIndex);
    const float3 wpos = ObjectToWorldPos(in_POSITION);
    const uint projectionIndex = light.LIGHT_PROJECTION;
    
    float4 vs_pos = 0.0f.xxxx;
    float4 vs_depth = 0.0f.xxxx;

    switch (light.LIGHT_TYPE)
    {
        case LIGHT_TYPE_POINT:
        {
            vs_depth = float4(wpos - light.LIGHT_POS, SHADOW_NEAR_BIAS);
            vs_pos = GetCubeClipPos(vs_depth.xyz, light.LIGHT_RADIUS, layer % 6);
        }
        break;
        case LIGHT_TYPE_SPOT:
        {
            float4x4 lightmatrix = PK_BUFFER_DATA(pk_LightMatrices, projectionIndex);
            vs_pos = mul(lightmatrix, float4(wpos, 1.0f));
            vs_depth.xyz = wpos - light.LIGHT_POS;
            vs_depth.w = SHADOW_NEAR_BIAS;
        }
        break;
        case LIGHT_TYPE_DIRECTIONAL:
        {
            float4x4 lightmatrix = PK_BUFFER_DATA(pk_LightMatrices, projectionIndex + layer);
            vs_pos = mul(lightmatrix, float4(wpos, 1.0f));
            vs_depth.x = (vs_pos.z / vs_pos.w) * light.LIGHT_RADIUS;
            vs_depth.w = SHADOW_NEAR_BIAS * (layer + 1);
        }
        break;
    }

    gl_Layer = int(layer);
    gl_Position = vs_pos;
    vs_DEPTH = vs_depth;
}

#pragma PROGRAM_FRAGMENT
in float4 vs_DEPTH;
layout(early_fragment_tests) in;
layout(location = 0) out float2 SV_Target0;

void main()
{
    float d = length(vs_DEPTH.xyz) + vs_DEPTH.w;
    SV_Target0 = float2(d, d * d);
};