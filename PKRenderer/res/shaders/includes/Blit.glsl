#pragma once
#if defined(SHADER_STAGE_VERTEX)

float4 PK_BLIT_VERTEX_POSITIONS[3] =
{
	float4(-1.0,  1.0, 1.0, 1.0),
	float4(-1.0, -3.0, 1.0, 1.0),
	float4( 3.0,  1.0, 1.0, 1.0),
}; 

float2 PK_BLIT_VERTEX_TEXCOORDS[3] =
{
	float2(0.0,  0.0),
	float2(0.0,  2.0),
	float2(2.0,  0.0),
}; 

#define PK_BLIT_VERTEX_POSITION PK_BLIT_VERTEX_POSITIONS[gl_VertexIndex]
#define PK_BLIT_VERTEX_TEXCOORD PK_BLIT_VERTEX_TEXCOORDS[gl_VertexIndex]

#endif