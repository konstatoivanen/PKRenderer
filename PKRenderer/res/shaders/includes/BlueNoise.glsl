#pragma once

PK_DECLARE_SET_GLOBAL uniform sampler2D pk_Bluenoise256;
PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_Bluenoise128x64;

float3 GlobalNoiseBlue(uint2 coord) { return texelFetch(pk_Bluenoise256, int2(coord.x % 256, coord.y % 256), 0).xyz; }
float3 GlobalNoiseBlueScreenUV(float2 coord) { return GlobalNoiseBlue(uint2(coord * pk_ScreenParams.xy)); }
float3 GlobalNoiseBlueUV(float2 coord) { return tex2D(pk_Bluenoise256, coord).xyz; }

float3 GlobalNoiseBlue(uint2 coord, uint layer) 
{
    coord.xy += (layer / 64u).xx * 7u;
    return texelFetch(pk_Bluenoise128x64, int3(coord.x % 128, coord.y % 128, layer % 64), 0).xyz; 
}

float3 GlobalNoiseBlueUV(float3 coord) { return tex2D(pk_Bluenoise128x64, coord).xyz; }