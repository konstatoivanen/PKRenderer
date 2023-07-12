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

// Dispatch 2d dimension must be divisible by 32
uint3 GetMortonOrderSwizzle32(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

    uint index0 = threadIndex >> 0u;
    uint index1 = threadIndex >> 2u;
    uint index2 = threadIndex >> 4u;
    uint index3 = threadIndex >> 6u;
    uint index4 = threadIndex >> 8u;

    uint2 coord = uint2(index0 % 2u, (index0 / 2u) % 2u) << 0u;
    coord += uint2(index1 % 2u, (index1 / 2u) % 2u) << 1u;
    coord += uint2(index2 % 2u, (index2 / 2u) % 2u) << 2u;
    coord += uint2(index3 % 2u, (index3 / 2u) % 2u) << 3u;

    size >>= 4u;
    coord += uint2(index4 % size.x, index4 / size.x) << 4u;

    return uint3(coord, z);
}

// Dispatch 2d dimension must be divisible by 16
uint3 GetMortonOrderSwizzle16(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

    uint index0 = threadIndex >> 0u;
    uint index1 = threadIndex >> 2u;
    uint index2 = threadIndex >> 4u;
    uint index3 = threadIndex >> 6u;

    uint2 coord = uint2(index0 % 2u, (index0 / 2u) % 2u) << 0u;
    coord += uint2(index1 % 2u, (index1 / 2u) % 2u) << 1u;
    coord += uint2(index2 % 2u, (index2 / 2u) % 2u) << 2u;

    size >>= 3u;
    coord += uint2(index3 % size.x, index3 / size.x) << 3u;

    return uint3(coord, z);
}

// Dispatch 2d dimension must be divisible by 8
uint3 GetMortonOrderSwizzle8(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

    uint index0 = threadIndex >> 0u;
    uint index1 = threadIndex >> 2u;
    uint index2 = threadIndex >> 4u;

    uint2 coord = uint2(index0 % 2u, (index0 / 2u) % 2u) << 0u;
    coord += uint2(index1 % 2u, (index1 / 2u) % 2u) << 1u;

    size >>= 2u;
    coord += uint2(index2 % size.x, index2 / size.x) << 2u;

    return uint3(coord, z);
}