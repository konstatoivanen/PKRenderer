#pragma once
#include "Common.glsl"
uniform texture2D pk_Bluenoise256;
uniform texture2DArray pk_Bluenoise128x64;
float3 GlobalNoiseBlue(uint2 coord) { return texelFetch(pk_Bluenoise256, int2(coord.x & 255u, coord.y & 255u), 0).xyz; }
float3 GlobalNoiseBlueUv(float2 coord) { return TMP_SMP_2D(pk_Bluenoise256, pk_SamplerBilinearRepeat, coord).xyz; }
float3 GlobalNoiseBlue(uint2 coord, uint layer) { return texelFetch(pk_Bluenoise128x64, int3(coord.x & 127u, coord.y & 127u, layer & 63u), 0).xyz; }
float3 GlobalNoiseBlueUv(float3 coord) { return TMP_SMP_2DArray(pk_Bluenoise128x64, pk_SamplerBilinearRepeat, coord).xyz; }
