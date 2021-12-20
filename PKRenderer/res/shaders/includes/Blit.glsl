#pragma once
#if defined(SHADER_STAGE_VERTEX)

vec4 PK_BLIT_VERTEX_POSITIONS[3] =
{
	vec4(-1.0,  1.0, 1.0, 1.0),
	vec4(-1.0, -3.0, 1.0, 1.0),
	vec4( 3.0,  1.0, 1.0, 1.0),
}; 

vec2 PK_BLIT_VERTEX_TEXCOORDS[3] =
{
	vec2(0.0,  0.0),
	vec2(0.0,  2.0),
	vec2(2.0,  0.0),
}; 

#define PK_BLIT_VERTEX_POSITION PK_BLIT_VERTEX_POSITIONS[gl_VertexIndex]
#define PK_BLIT_VERTEX_TEXCOORD PK_BLIT_VERTEX_TEXCOORDS[gl_VertexIndex]

#endif