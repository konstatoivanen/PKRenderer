#version 460
#BlendColor Add One SrcAlpha
#include includes/SharedVolumeFog.glsl
#pragma PROGRAM_VERTEX
#include includes/Blit.glsl
out float4 vs_TEXCOORD;

void main()
{
    vs_TEXCOORD = PK_BLIT_VERTEX_TEXCOORD.xyxy;
    vs_TEXCOORD = float4(vs_TEXCOORD.xy, vs_TEXCOORD.xy * pk_ScreenParams.xy + pk_Time.ww * 1000);
    gl_Position = PK_BLIT_VERTEX_POSITION;
};

#pragma PROGRAM_FRAGMENT

// Source: https://www.shadertoy.com/view/cts3Rj
float4 SampleTriCubic(float3 coord)
{
	float3 texSize = textureSize(pk_Volume_ScatterRead, 0).xyz;

	// Shift the coordinate from [0,1] to [-0.5, textureSize-0.5]
	float3 coord_grid = coord * texSize - 0.5;
	float3 index = floor(coord_grid);
	float3 fraction = coord_grid - index;
	float3 one_frac = 1.0 - fraction;

	float3 w0 = 1.0 / 6.0 * one_frac * one_frac * one_frac;
	float3 w1 = 2.0 / 3.0 - 0.5 * fraction * fraction * (2.0 - fraction);
	float3 w2 = 2.0 / 3.0 - 0.5 * one_frac * one_frac * (2.0 - one_frac);
	float3 w3 = 1.0 / 6.0 * fraction * fraction * fraction;

	float3 g0 = w0 + w1;
	float3 g1 = w2 + w3;
	float3 mult = 1.0 / texSize;
	float3 h0 = mult * ((w1 / g0) - 0.5 + index); //h0 = w1/g0 - 1, move from [-0.5, textureSize-0.5] to [0,1]
	float3 h1 = mult * ((w3 / g1) + 1.5 + index); //h1 = w3/g1 + 1, move from [-0.5, textureSize-0.5] to [0,1]

	// Fetch the eight linear interpolations
	// Weighting and fetching is interleaved for performance and stability reasons
	float4 tex000 = textureLod(pk_Volume_ScatterRead, h0, 0.);
	float4 tex100 = textureLod(pk_Volume_ScatterRead, vec3(h1.x, h0.y, h0.z), 0.0);
	tex000 = mix(tex100, tex000, g0.x); // Weigh along the x-direction

	float4 tex010 = textureLod(pk_Volume_ScatterRead, vec3(h0.x, h1.y, h0.z), 0.0);
	float4 tex110 = textureLod(pk_Volume_ScatterRead, vec3(h1.x, h1.y, h0.z), 0.0);
	tex010 = mix(tex110, tex010, g0.x); // Weigh along the x-direction
	tex000 = mix(tex010, tex000, g0.y); // Weigh along the y-direction

	float4 tex001 = textureLod(pk_Volume_ScatterRead, vec3(h0.x, h0.y, h1.z), 0.0);
	float4 tex101 = textureLod(pk_Volume_ScatterRead, vec3(h1.x, h0.y, h1.z), 0.0);
	tex001 = mix(tex101, tex001, g0.x); // Weigh along the x-direction

	float4 tex011 = textureLod(pk_Volume_ScatterRead, vec3(h0.x, h1.y, h1.z), 0.0);
	float4 tex111 = textureLod(pk_Volume_ScatterRead, h1, 0.0);
	tex011 = mix(tex111, tex011, g0.x); // Weigh along the x-direction
	tex001 = mix(tex011, tex001, g0.y); // Weigh along the y-direction

	return mix(tex001, tex000, g0.z); // Weigh along the z-direction
}

in float4 vs_TEXCOORD;
out float4 SV_Target0;
void main()
{
    float d = SampleLinearDepth(vs_TEXCOORD.xy);
    float w = GetVolumeWCoord(d);

    float3 offset = GlobalNoiseBlue(uint2(vs_TEXCOORD.zw), pk_FrameIndex.x);
    offset -= 0.5f;
    offset *= VOLUME_COMPOSITE_DITHER_AMOUNT;
    SV_Target0 = SampleTriCubic(float3(vs_TEXCOORD.xy, w) + offset.xyz);
};