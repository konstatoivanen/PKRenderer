#pragma once
#include Utilities.glsl

#define LIGHT_PARAM_INVALID 0x7FFF
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

#define LIGHT_SHADOW i.x >> 16u 
#define LIGHT_PROJECTION i.x & 0xFFFFu
#define LIGHT_COOKIE i.y >> 16u
#define LIGHT_TYPE i.y & 0xFFFFu
#define LIGHT_PACKED_DIRECTION i.z
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
};

struct LightTile 
{
    uint start;
    uint end;
    uint cascade;
};

struct LightPacked { float4 p; float4 c; uint4 i; };

PK_DECLARE_SET_GLOBAL uniform sampler2DArray pk_LightCookies;
PK_DECLARE_SET_PASS uniform sampler2DArray pk_ShadowmapAtlas;
PK_DECLARE_READONLY_BUFFER(LightPacked, pk_Lights, PK_SET_PASS);
PK_DECLARE_READONLY_BUFFER(float4x4, pk_LightMatrices, PK_SET_PASS);

#if defined(PK_WRITE_LIGHT_CLUSTERS)
    PK_DECLARE_WRITEONLY_BUFFER(uint, pk_GlobalLightsList, PK_SET_PASS);
    layout(r32ui, set = PK_SET_PASS) uniform writeonly uimage3D pk_LightTiles;
#else
    PK_DECLARE_READONLY_BUFFER(uint, pk_GlobalLightsList, PK_SET_PASS);
    layout(r32ui, set = PK_SET_PASS) uniform readonly uimage3D pk_LightTiles;
#endif

LightTile CreateLightTile(uint data)
{
	uint offset = bitfieldExtract(data, 0, 20);
	uint count = bitfieldExtract(data, 20, 8);
    uint cascade = bitfieldExtract(data, 28, 4);
    return LightTile(offset, offset + count, cascade);
}

LightTile GetLightTile(int3 coord)
{
    #if defined(PK_WRITE_LIGHT_CLUSTERS)
        return LightTile(0,0,0);
    #else
        return CreateLightTile(imageLoad(pk_LightTiles, coord).x);
    #endif
}
