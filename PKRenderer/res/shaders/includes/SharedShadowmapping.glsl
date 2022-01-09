#pragma once
#include SharedLights.glsl
#include Encoding.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_ShadowmapData)
{
    uint pk_ShadowmapLightIndex;
    uint pk_ShadowmapBaseLayer;
    float4 pk_ShadowmapBlurAmount;
};

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
	return float4(vpos.xy, 1.020202f * vpos.z - radius * 0.020202f, vpos.z);
}