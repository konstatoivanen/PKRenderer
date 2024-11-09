#pragma once
PK_DECLARE_SET_GLOBAL uniform sampler2D pk_Bluenoise256;
PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_Bluenoise128x64;
float3 GlobalNoiseBlue(uint2 coord) { return texelFetch(pk_Bluenoise256, int2(coord.x & 255u, coord.y & 255u), 0).xyz; }
float3 GlobalNoiseBlueUV(float2 coord) { return texture(pk_Bluenoise256, coord).xyz; }
float3 GlobalNoiseBlue(uint2 coord, uint layer) { return texelFetch(pk_Bluenoise128x64, int3(coord.x & 127u, coord.y & 127u, layer & 63u), 0).xyz; }
float3 GlobalNoiseBlueUV(float3 coord) { return texture(pk_Bluenoise128x64, coord).xyz; }
