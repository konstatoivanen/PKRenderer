#pragma once
#include "Common.glsl"

#define LIGHT_TILE_SIZE_PX 64
#define LIGHT_TILE_SHIFT_PX 6
#define LIGHT_TILE_COUNT_Z 32
#define LIGHT_TILE_COUNT_XY imageSize(pk_LightTiles).xy
#define LIGHT_TILE_MAX_LIGHTS 128

#define LIGHT_PARAM_INVALID 0x7FFF
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_POINT 2

#define LIGHT_SHADOW i.x & 0xFFFFu
#define LIGHT_MATRIX i.x >> 16u 
#define LIGHT_TYPE i.y & 0xFFFFu
#define LIGHT_COOKIE i.y >> 16u
#define LIGHT_PACKED_DIRECTION i.z
#define LIGHT_PACKED_SOURCERADIUS i.w
#define LIGHT_POS p.xyz
#define LIGHT_RADIUS p.w
#define LIGHT_COLOR c.xyz
#define LIGHT_ANGLE c.w

struct Light
{
    float3 color;
    float shadow;
    float3 direction;
    float linearDistance;
    float sourceRadius;
};

struct LightTile 
{
    uint start;
    uint end;
    uint cascade;
};

struct LightPacked { float4 p; float4 c; uint4 i; };

PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_LightCookies;
PK_DECLARE_READONLY_BUFFER(uint4, pk_Lights, PK_SET_PASS);
PK_DECLARE_READONLY_BUFFER(float4x4, pk_LightMatrices, PK_SET_PASS);

#if defined(PK_WRITE_LIGHT_CLUSTERS)
    PK_DECLARE_WRITEONLY_BUFFER(ushort, pk_LightLists, PK_SET_PASS);
    layout(r32ui, set = PK_SET_PASS) uniform writeonly uimage3D pk_LightTiles;
#else
    PK_DECLARE_READONLY_BUFFER(ushort, pk_LightLists, PK_SET_PASS);
    layout(r32ui, set = PK_SET_PASS) uniform readonly uimage3D pk_LightTiles;
#endif

LightPacked Lights_LoadPacked(uint index)
{
    LightPacked l;
    l.p = uintBitsToFloat(PK_BUFFER_DATA(pk_Lights, index * 3u));
    l.c = uintBitsToFloat(PK_BUFFER_DATA(pk_Lights, index * 3u + 1u));
    l.i = PK_BUFFER_DATA(pk_Lights, index * 3u + 2u);
    return l;
}

LightTile Lights_CreateTile(uint data)
{
    uint offset = bitfieldExtract(data, 0, 22);
    uint count = bitfieldExtract(data, 22, 8);
    uint cascade = bitfieldExtract(data, 30, 2);
    return LightTile(offset, offset + count, cascade);
}

LightTile Lights_GetTile(const int3 coord)
{
    #if defined(PK_WRITE_LIGHT_CLUSTERS)
        return LightTile(0,0,0);
    #else
        return Lights_CreateTile(imageLoad(pk_LightTiles, coord).x);
    #endif
}

LightTile Lights_GetTile_COORD(const int2 coord, const float viewdepth) { return Lights_GetTile(int3(coord, max(0, int(LIGHT_TILE_COUNT_Z * ClipDepthExp(viewdepth))))); }
LightTile Lights_GetTile_PX(const int2 px, const float viewdepth) { return Lights_GetTile_COORD(px >> LIGHT_TILE_SHIFT_PX, viewdepth); }
LightTile Lights_GetTile_UV(const float2 uv, const float viewdepth) { return Lights_GetTile_PX(int2(uv * pk_ScreenSize.xy), viewdepth); }
