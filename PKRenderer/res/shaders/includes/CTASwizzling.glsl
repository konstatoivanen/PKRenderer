#pragma once

#define DXC_STATIC_DISPATCH_GRID_DIM 0

// Source: https://github.com/LouisBavoil/ThreadGroupIDSwizzling/blob/master/ThreadGroupTilingX.hlsl
// Divide the 2D-Dispatch_Grid into tiles of dimension [N, DipatchGridDim.y]
// “CTA” (Cooperative Thread Array) == Thread Group in DirectX terminology
uint2 ThreadGroupTilingX(
	const uint2 dipatchGridDim,		// Arguments of the Dispatch call (typically from a ConstantBuffer)
	const uint2 ctaDim,			// Already known in HLSL, eg:[numthreads(8, 8, 1)] -> uint2(8, 8)
	const uint maxTileWidth,		// User parameter (N). Recommended values: 8, 16 or 32.
	const uint2 groupThreadID,		// SV_GroupThreadID
	const uint2 groupId			// SV_GroupID
)
{
	// A perfect tile is one with dimensions = [maxTileWidth, dipatchGridDim.y]
	const uint Number_of_CTAs_in_a_perfect_tile = maxTileWidth * dipatchGridDim.y;

	// Possible number of perfect tiles
	const uint Number_of_perfect_tiles = dipatchGridDim.x / maxTileWidth;

	// Total number of CTAs present in the perfect tiles
	const uint Total_CTAs_in_all_perfect_tiles = Number_of_perfect_tiles * maxTileWidth * dipatchGridDim.y;
	const uint vThreadGroupIDFlattened = dipatchGridDim.x * groupId.y + groupId.x;

	// Tile_ID_of_current_CTA : current CTA to TILE-ID mapping.
	const uint Tile_ID_of_current_CTA = vThreadGroupIDFlattened / Number_of_CTAs_in_a_perfect_tile;
	const uint Local_CTA_ID_within_current_tile = vThreadGroupIDFlattened % Number_of_CTAs_in_a_perfect_tile;
	uint Local_CTA_ID_y_within_current_tile;
	uint Local_CTA_ID_x_within_current_tile;

	if (Total_CTAs_in_all_perfect_tiles <= vThreadGroupIDFlattened)
	{
		// Path taken only if the last tile has imperfect dimensions and CTAs from the last tile are launched. 
		uint X_dimension_of_last_tile = dipatchGridDim.x % maxTileWidth;
	#ifdef DXC_STATIC_DISPATCH_GRID_DIM
		X_dimension_of_last_tile = max(1, X_dimension_of_last_tile);
	#endif
		Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / X_dimension_of_last_tile;
		Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % X_dimension_of_last_tile;
	}
	else
	{
		Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / maxTileWidth;
		Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % maxTileWidth;
	}

	const uint Swizzled_vThreadGroupIDFlattened =
		Tile_ID_of_current_CTA * maxTileWidth +
		Local_CTA_ID_y_within_current_tile * dipatchGridDim.x +
		Local_CTA_ID_x_within_current_tile;

	uint2 SwizzledvThreadGroupID;
	SwizzledvThreadGroupID.y = Swizzled_vThreadGroupIDFlattened / dipatchGridDim.x;
	SwizzledvThreadGroupID.x = Swizzled_vThreadGroupIDFlattened % dipatchGridDim.x;

	uint2 SwizzledvThreadID;
	SwizzledvThreadID.x = ctaDim.x * SwizzledvThreadGroupID.x + groupThreadID.x;
	SwizzledvThreadID.y = ctaDim.y * SwizzledvThreadGroupID.y + groupThreadID.y;

	return SwizzledvThreadID.xy;
}

#define GetXTiledThreadID(x, y, width) ThreadGroupTilingX(gl_NumWorkGroups.xy, uint2(x,y), width, gl_LocalInvocationID.xy, gl_WorkGroupID.xy)

// Source: https://gist.github.com/franjaviersans/885c136932ef37d8905a6433d0828be6
uint IntegerCompact2(uint x)
{
    x = (x & 0x11111111) | ((x & 0x44444444) >> 1);
    x = (x & 0x03030303) | ((x & 0x30303030) >> 2);
    x = (x & 0x000F000F) | ((x & 0x0F000F00) >> 4);
    x = (x & 0x000000FF) | ((x & 0x00FF0000) >> 8);
    return x;
}

uint IntegerCompact3(uint n)
{
	n &= 0x09249249;
	n = (n ^ (n >> 2)) & 0x030c30c3;
	n = (n ^ (n >> 4)) & 0x0300f00f;
	n = (n ^ (n >> 8)) & 0xff0000ff;
	n = (n ^ (n >> 16)) & 0x000003ff;
	return n;
}

uint2 GetMortonOrderSwizzle2D(uint x) { return uint2(IntegerCompact2(x), IntegerCompact2(x >> 1u)); }
uint3 GetMortonOrderSwizzle3D(uint x) { return uint3(IntegerCompact3(x), IntegerCompact3(x >> 1u), IntegerCompact3(x >> 2u)); }

// Dispatch 2d dimension must be divisible by 32
uint3 GetMortonOrderSwizzle32(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

	uint2 coord = GetMortonOrderSwizzle2D(threadIndex & 0x3FFu);
	uint groupIndex = threadIndex >> 10u;
	coord.x += (groupIndex % (size.x >> 5u)) << 5u;
	coord.y += (groupIndex / (size.x >> 5u)) << 5u;
    return uint3(coord, z);
}

// Dispatch 2d dimension must be divisible by 16
uint3 GetMortonOrderSwizzle16(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

	uint2 coord = GetMortonOrderSwizzle2D(threadIndex & 0xFFu);
	uint groupIndex = threadIndex >> 8u;
	coord.x += (groupIndex % (size.x >> 4u)) << 4u;
	coord.y += (groupIndex / (size.x >> 4u)) << 4u;
    return uint3(coord, z);
}

// Dispatch 2d dimension must be divisible by 8
uint3 GetMortonOrderSwizzle8(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

	uint2 coord = GetMortonOrderSwizzle2D(threadIndex & 0x3Fu);
	uint groupIndex = threadIndex >> 6u;
	coord.x += (groupIndex % (size.x >> 3u)) << 3u;
	coord.y += (groupIndex / (size.x >> 3u)) << 3u;
    return uint3(coord, z);
}