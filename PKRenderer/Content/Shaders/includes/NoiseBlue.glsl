#pragma once
#include "Common.glsl"
uniform Texture2D<float3> pk_Bluenoise256;
uniform Texture2DArray<float3> pk_Bluenoise128x64;
float3 GlobalNoiseBlue(uint2 coord) { return pk_Bluenoise256[int2(coord.x & 255u, coord.y & 255u)]; }
float3 GlobalNoiseBlueUv(float2 coord) { return pk_Bluenoise256.Sample(pk_SamplerBilinearRepeat, coord); }
float3 GlobalNoiseBlue(uint2 coord, uint layer) { return pk_Bluenoise128x64.Load(int4(coord.x & 127u, coord.y & 127u, layer & 63u, 0)); }
float3 GlobalNoiseBlueUv(float3 coord) { return pk_Bluenoise128x64.Sample(pk_SamplerBilinearRepeat, coord); }
