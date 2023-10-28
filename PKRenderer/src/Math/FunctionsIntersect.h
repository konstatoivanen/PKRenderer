#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    void NormalizePlane(float4* plane);
    FrustumPlanes ExtractFrustrumPlanes(const float4x4 viewToClip, bool normalize);
    float PlaneMaxDistanceToAABB(const float4& plane, const BoundingBox& aabb);
    float PlaneMinDistanceToAABB(const float4& plane, const BoundingBox& aabb);
    float PlaneDistanceToPoint(const float4& plane, const float3& point);
    float3 IntesectPlanes3(const float4& p1, const float4& p2, const float4& p3);
    bool IntersectPlanesAABB(const float4* planes, uint32_t planeCount, const BoundingBox& aabb);
    bool IntersectAABB(const BoundingBox& a, const BoundingBox& b);
    bool IntersectSphere(const float3& center, float radius, const BoundingBox& b);
    void BoundsEncapsulate(BoundingBox* bounds, const BoundingBox& other);
    void BoundsEncapsulate(BoundingBox* bounds, float* bmin, float* bmax);
    uint32_t BoundsLongestAxis(const BoundingBox& bounds);
    uint32_t BoundsShortestAxis(const BoundingBox& bounds);
    void BoundsSplit(const BoundingBox& bounds, uint32_t axis, BoundingBox* out0, BoundingBox* out1);
    bool BoundsContains(const BoundingBox& bounds, const float3& point);
    BoundingBox BoundsTransform(const float4x4& matrix, const BoundingBox& bounds);
    BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix);
    BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix, float lznear, float lzfar);
    BoundingBox GetInverseFrustumBounds(const float4x4& worldToLocal, const float4x4& inverseMatrix);
}