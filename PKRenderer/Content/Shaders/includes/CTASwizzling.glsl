#pragma once
#include MortonOrder.glsl

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

// Dispatch 2d dimension must be divisible by 32
uint3 GetZCurveSwizzle32(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

    uint2 coord = IndexToZCurve2D(threadIndex & 0x3FFu);
    uint groupIndex = threadIndex >> 10u;
    coord.x += (groupIndex % (size.x >> 5u)) << 5u;
    coord.y += (groupIndex / (size.x >> 5u)) << 5u;
    return uint3(coord, z);
}

// Dispatch 2d dimension must be divisible by 16
uint3 GetZCurveSwizzle16(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

    uint2 coord = IndexToZCurve2D(threadIndex & 0xFFu);
    uint groupIndex = threadIndex >> 8u;
    coord.x += (groupIndex % (size.x >> 4u)) << 4u;
    coord.y += (groupIndex / (size.x >> 4u)) << 4u;
    return uint3(coord, z);
}

// Dispatch 2d dimension must be divisible by 8
uint3 GetZCurveSwizzle8(uint threadIndex, uint2 size)
{
    uint z = threadIndex / (size.x * size.y);
    threadIndex -= z * (size.x * size.y);

    uint2 coord = IndexToZCurve2D(threadIndex & 0x3Fu);
    uint groupIndex = threadIndex >> 6u;
    coord.x += (groupIndex % (size.x >> 3u)) << 3u;
    coord.y += (groupIndex / (size.x >> 3u)) << 3u;
    return uint3(coord, z);
}