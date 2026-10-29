#pragma once
uniform Texture2DRef pk_Bluenoise256;
uniform Texture2DArrayRef pk_Bluenoise128x64;
float3 GlobalNoiseBlue(uint2 coord) { return pk_Bluenoise256[int2(coord.x & 255u, coord.y & 255u)].xyz; }
float3 GlobalNoiseBlueUv(float2 coord) { return pk_Bluenoise256.Sample(pk_SamplerBilinearRepeat, coord).xyz; } 
float3 GlobalNoiseBlue(uint2 coord, uint layer) { return pk_Bluenoise128x64.Load(int3(coord.x & 127u, coord.y & 127u, layer & 63u), 0).xyz; }
float3 GlobalNoiseBlueUv(float3 coord) { return pk_Bluenoise128x64.Sample(pk_SamplerBilinearRepeat, coord).xyz; } 
