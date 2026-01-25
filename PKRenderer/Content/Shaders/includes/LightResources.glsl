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

struct SceneLight
{
    float3 position;
    float3 color;
    float3 direction;
    float2 spot_angles;
    float radius;
    float source_radius;
    uint light_type; 
    uint index_mask;
    uint index_matrix; 
    uint index_shadow; 
};

struct SceneLightSample
{
    float3 color;
    float shadow;
    float3 direction;
    float linear_distance;
    float source_radius;
};

struct LightTile 
{
    uint start;
    uint end;
    uint cascade;
};

PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_LightCookies;
PK_DECLARE_READONLY_BUFFER(uint4, pk_Lights, PK_SET_PASS);
PK_DECLARE_READONLY_BUFFER(float4x4, pk_LightMatrices, PK_SET_PASS);

#if defined(PK_WRITE_LIGHT_CLUSTERS)
    PK_DECLARE_WRITEONLY_BUFFER(ushort, pk_LightLists, PK_SET_PASS);
    PK_DECLARE_SET_PASS uniform writeonly uimage3D pk_LightTiles;
#else
    PK_DECLARE_READONLY_BUFFER(ushort, pk_LightLists, PK_SET_PASS);
    PK_DECLARE_SET_PASS uniform readonly uimage3D pk_LightTiles;
#endif

SceneLight Lights_UnpackLight(uint4 packed0, uint4 packed1)
{
    SceneLight l;
    l.position.xy = unpackHalf2x16(packed0.x);
    l.position.z = unpackHalf2x16(packed0.y).x;
    l.radius = unpackHalf2x16(packed0.y).y;
    l.color.xy = unpackHalf2x16(packed0.z);
    l.color.z = unpackHalf2x16(packed0.w).x;
    l.light_type = bitfieldExtract(packed0.w, 16, 4);
    l.index_mask = bitfieldExtract(packed0.w, 20, 12);
    l.direction.xy = unpackHalf2x16(packed1.x);
    l.direction.z = unpackHalf2x16(packed1.y).x;
    l.source_radius = unpackHalf2x16(packed1.y).y;
    l.spot_angles = unpackHalf2x16(packed1.z);
    l.index_shadow = bitfieldExtract(packed1.w, 0, 16);
    l.index_matrix = bitfieldExtract(packed1.w, 16, 16);
    return l;
}

LightTile Lights_UnpackTile(uint data)
{
    const uint offset = bitfieldExtract(data, 0, 22);
    const uint count = bitfieldExtract(data, 22, 8);
    const uint cascade = bitfieldExtract(data, 30, 2);
    return LightTile(offset, offset + count, cascade);
}

SceneLight Lights_LoadLight(uint index)
{
    return Lights_UnpackLight(PK_BUFFER_DATA(pk_Lights, index * 2u + 0u), PK_BUFFER_DATA(pk_Lights, index * 2u + 1u));
}

LightTile Lights_LoadTile(const int3 coord)
{
    #if defined(PK_WRITE_LIGHT_CLUSTERS)
        return LightTile(0,0,0);
    #else
        return Lights_UnpackTile(imageLoad(pk_LightTiles, coord).x);
    #endif
}

LightTile Lights_LoadTile_Coord(const int2 coord, const float view_depth) { return Lights_LoadTile(int3(coord, max(0, int(ClipDepthExp(view_depth, pk_LightTileZParams.xyz))))); }
LightTile Lights_LoadTile_Px(const int2 px, const float view_depth) { return Lights_LoadTile_Coord(px >> LIGHT_TILE_SHIFT_PX, view_depth); }
LightTile Lights_LoadTile_Uv(const float2 uv, const float view_depth) { return Lights_LoadTile_Px(int2(uv * pk_ScreenSize.xy), view_depth); }
