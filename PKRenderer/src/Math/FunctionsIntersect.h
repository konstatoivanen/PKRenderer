#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    void NormalizePlane(float4* plane);
    void ExtractFrustrumPlanes(const float4x4 viewprojection, FrustumPlanes* frustrum, bool normalize);
    float PlaneDistanceToAABB(const float4& plane, const BoundingBox& aabb);
    float PlaneDistanceToPoint(const float4& plane, const float3& point);
    float3 IntesectPlanes3(const float4& p1, const float4& p2, const float4& p3);
    bool IntersectPlanesAABB(const float4* planes, int planeCount, const BoundingBox& aabb);
    bool IntersectAABB(const BoundingBox& a, const BoundingBox& b);
    bool IntersectSphere(const float3& center, float radius, const BoundingBox& b);
    void BoundsEncapsulate(BoundingBox* bounds, const BoundingBox& other);
    void BoundsEncapsulate(BoundingBox* bounds, float* bmin, float* bmax);
    int BoundsLongestAxis(const BoundingBox& bounds);
    int BoundsShortestAxis(const BoundingBox& bounds);
    void BoundsSplit(const BoundingBox& bounds, int axis, BoundingBox* out0, BoundingBox* out1);
    bool BoundsContains(const BoundingBox& bounds, const float3& point);
    BoundingBox BoundsTransform(const float4x4& matrix, const BoundingBox& bounds);
    BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix);
    BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix, float lznear, float lzfar);
    BoundingBox GetInverseFrustumBounds(const float4x4& worldToLocal, const float4x4& inverseMatrix);
}