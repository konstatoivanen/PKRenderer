#include "PrecompiledHeader.h"
#include "FunctionsIntersect.h"

namespace PK::Math::Functions
{
    void NormalizePlane(float4* plane)
    {
        float mag;
        mag = sqrt(plane->x * plane->x + plane->y * plane->y + plane->z * plane->z);
        plane->x = plane->x / mag;
        plane->y = plane->y / mag;
        plane->z = plane->z / mag;
        plane->w = plane->w / mag;
    }

    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    // DirectX convention
    FrustumPlanes ExtractFrustrumPlanes(const float4x4 viewToClip, bool normalize)
    {
        FrustumPlanes planes;

        // Left clipping plane
        planes[0].x = viewToClip[0][3] + viewToClip[0][0];
        planes[0].y = viewToClip[1][3] + viewToClip[1][0];
        planes[0].z = viewToClip[2][3] + viewToClip[2][0];
        planes[0].w = viewToClip[3][3] + viewToClip[3][0];
        // Right clipping plane
        planes[1].x = viewToClip[0][3] - viewToClip[0][0];
        planes[1].y = viewToClip[1][3] - viewToClip[1][0];
        planes[1].z = viewToClip[2][3] - viewToClip[2][0];
        planes[1].w = viewToClip[3][3] - viewToClip[3][0];
        // Top clipping plane
        planes[2].x = viewToClip[0][3] - viewToClip[0][1];
        planes[2].y = viewToClip[1][3] - viewToClip[1][1];
        planes[2].z = viewToClip[2][3] - viewToClip[2][1];
        planes[2].w = viewToClip[3][3] - viewToClip[3][1];
        // Bottom clipping plane
        planes[3].x = viewToClip[0][3] + viewToClip[0][1];
        planes[3].y = viewToClip[1][3] + viewToClip[1][1];
        planes[3].z = viewToClip[2][3] + viewToClip[2][1];
        planes[3].w = viewToClip[3][3] + viewToClip[3][1];
        // Near clipping plane
        planes[4].x = viewToClip[0][2];
        planes[4].y = viewToClip[1][2];
        planes[4].z = viewToClip[2][2];
        planes[4].w = viewToClip[3][2];
        // Far clipping plane
        planes[5].x = viewToClip[0][3] - viewToClip[0][2];
        planes[5].y = viewToClip[1][3] - viewToClip[1][2];
        planes[5].z = viewToClip[2][3] - viewToClip[2][2];
        planes[5].w = viewToClip[3][3] - viewToClip[3][2];

        // Normalize the plane equations, if requested
        if (normalize)
        {
            NormalizePlane(&planes[0]);
            NormalizePlane(&planes[1]);
            NormalizePlane(&planes[2]);
            NormalizePlane(&planes[3]);
            NormalizePlane(&planes[4]);
            NormalizePlane(&planes[5]);
        }

        return planes;
    }

    float PlaneMaxDistanceToAABB(const float4& plane, const BoundingBox& aabb)
    {
        auto bx = plane.x > 0 ? aabb.max.x : aabb.min.x;
        auto by = plane.y > 0 ? aabb.max.y : aabb.min.y;
        auto bz = plane.z > 0 ? aabb.max.z : aabb.min.z;
        return plane.x * bx + plane.y * by + plane.z * bz + plane.w;
    }

    float PlaneMinDistanceToAABB(const float4& plane, const BoundingBox& aabb)
    {
        auto bx = plane.x < 0 ? aabb.max.x : aabb.min.x;
        auto by = plane.y < 0 ? aabb.max.y : aabb.min.y;
        auto bz = plane.z < 0 ? aabb.max.z : aabb.min.z;
        return plane.x * bx + plane.y * by + plane.z * bz + plane.w;
    }

    bool IntersectPlanesAABB(const float4* planes, uint32_t planeCount, const BoundingBox& aabb)
    {
        for (auto i = 0u; i < planeCount; ++i)
        {
            auto& plane = planes[i];

            auto bx = plane.x > 0 ? aabb.max.x : aabb.min.x;
            auto by = plane.y > 0 ? aabb.max.y : aabb.min.y;
            auto bz = plane.z > 0 ? aabb.max.z : aabb.min.z;

            if (plane.x * bx + plane.y * by + plane.z * bz < -plane.w)
            {
                return false;
            }
        }

        return true;
    }

    float PlaneDistanceToPoint(const float4& plane, const float3& point)
    {
        return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
    }

    float3 IntesectPlanes3(const float4& p1, const float4& p2, const float4& p3)
    {
        float3 n1 = p1.xyz, n2 = p2.xyz, n3 = p3.xyz;
        return ((-p1.w * glm::cross(n2, n3)) + (-p2.w * glm::cross(n3, n1)) + (-p3.w * glm::cross(n1, n2))) / (glm::dot(n1, glm::cross(n2, n3)));
    }

    bool IntersectAABB(const BoundingBox& a, const BoundingBox& b)
    {
        auto overlap = (!(a.min[0] > b.max[0]) && !(a.max[0] < b.min[0]));
        overlap = (!(a.min[1] > b.max[1]) && !(a.max[1] < b.min[1])) && overlap;
        overlap = (!(a.min[2] > b.max[2]) && !(a.max[2] < b.min[2])) && overlap;
        return overlap;
    }

    bool IntersectSphere(const float3& center, float radius, const BoundingBox& b)
    {
        float3 d = glm::abs(center - b.GetCenter()) - b.GetExtents();
        float r = radius - glm::compMax(min(d, float3(0.0f)));
        d = max(d, float3(0.0f));
        return radius > 0.0f && dot(d, d) <= r * r;
    }

    void BoundsEncapsulate(BoundingBox* bounds, const BoundingBox& other)
    {
        for (auto i = 0; i < 3; ++i)
        {
            if (other.min[i] < bounds->min[i])
            {
                bounds->min[i] = other.min[i];
            }

            if (other.max[i] > bounds->max[i])
            {
                bounds->max[i] = other.max[i];
            }
        }
    }

    void BoundsEncapsulate(BoundingBox* bounds, float* bmin, float* bmax)
    {
        for (auto i = 0; i < 3; ++i)
        {
            if (bmin[i] < bounds->min[i])
            {
                bounds->min[i] = bmin[i];
            }

            if (bmax[i] > bounds->max[i])
            {
                bounds->max[i] = bmax[i];
            }
        }
    }

    uint32_t BoundsLongestAxis(const BoundingBox& bounds)
    {
        auto ext = bounds.GetExtents();
        auto length = ext[0];
        auto axis = 0u;

        for (auto i = 1u; i < 3u; ++i)
        {
            if (ext[i] > length)
            {
                axis = i;
                length = ext[i];
            }
        }

        return axis;
    }

    uint32_t BoundsShortestAxis(const BoundingBox& bounds)
    {
        auto ext = bounds.GetExtents();
        auto length = ext[0];
        auto axis = 0u;

        for (auto i = 1u; i < 3u; ++i)
        {
            if (ext[i] < length)
            {
                axis = i;
                length = ext[i];
            }
        }

        return axis;
    }

    void BoundsSplit(const BoundingBox& bounds, uint32_t axis, BoundingBox* out0, BoundingBox* out1)
    {
        *out0 = bounds;
        *out1 = bounds;
        out0->max[axis] = bounds.min[axis] + (bounds.max[axis] - bounds.min[axis]) * 0.5f;
        out1->min[axis] = bounds.min[axis] + (bounds.max[axis] - bounds.min[axis]) * 0.5f;
    }

    bool BoundsContains(const BoundingBox& bounds, const float3& point)
    {
        for (auto i = 0; i < 3; ++i)
        {
            if (bounds.min[i] > point[i] || bounds.max[i] < point[i])
            {
                return false;
            }
        }

        return true;
    }

    BoundingBox BoundsTransform(const float4x4& matrix, const BoundingBox& bounds)
    {
        BoundingBox out(matrix[3].xyz, matrix[3].xyz);

        for (auto i = 0u; i < 3u; ++i)
        for (auto j = 0u; j < 3u; ++j)
        {
            auto a = matrix[j][i] * bounds.min[j];
            auto b = matrix[j][i] * bounds.max[j];

            if (a < b)
            {
                out.min[i] += a;
                out.max[i] += b;
            }
            else
            {
                out.min[i] += b;
                out.max[i] += a;
            }
        }

        return out;
    }

    BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix)
    {
        float4 positions[8];
        positions[0] = inverseMatrix * float4(-1, -1, PK_CLIPZ_NEAR, 1);
        positions[1] = inverseMatrix * float4(-1,  1, PK_CLIPZ_NEAR, 1);
        positions[2] = inverseMatrix * float4( 1,  1, PK_CLIPZ_NEAR, 1);
        positions[3] = inverseMatrix * float4( 1, -1, PK_CLIPZ_NEAR, 1);
        positions[4] = inverseMatrix * float4(-1, -1, PK_CLIPZ_FAR, 1);
        positions[5] = inverseMatrix * float4(-1,  1, PK_CLIPZ_FAR, 1);
        positions[6] = inverseMatrix * float4( 1,  1, PK_CLIPZ_FAR, 1);
        positions[7] = inverseMatrix * float4( 1, -1, PK_CLIPZ_FAR, 1);
        float3 min = { std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max() };
        float3 max = { -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max() };

        for (auto i = 0; i < 8; ++i)
        {
            positions[i] /= positions[i].w;
            max = glm::max(float3(positions[i].xyz), max);
            min = glm::min(float3(positions[i].xyz), min);
        }

        return BoundingBox::MinMax(min, max);
    }

    BoundingBox GetInverseFrustumBounds(const float4x4& inverseMatrix, float lznear, float lzfar)
    {
        float4 positions[8];
        positions[0] = inverseMatrix * float4(-1, -1, PK_CLIPZ_NEAR, 1);
        positions[1] = inverseMatrix * float4(-1,  1, PK_CLIPZ_NEAR, 1);
        positions[2] = inverseMatrix * float4( 1,  1, PK_CLIPZ_NEAR, 1);
        positions[3] = inverseMatrix * float4( 1, -1, PK_CLIPZ_NEAR, 1);
        positions[4] = inverseMatrix * float4(-1, -1, PK_CLIPZ_FAR, 1);
        positions[5] = inverseMatrix * float4(-1,  1, PK_CLIPZ_FAR, 1);
        positions[6] = inverseMatrix * float4( 1,  1, PK_CLIPZ_FAR, 1);
        positions[7] = inverseMatrix * float4( 1, -1, PK_CLIPZ_FAR, 1);

        for (auto i = 0; i < 4; ++i)
        {
            positions[i] /= positions[i].w;
            positions[i + 4] /= positions[i + 4].w;

            auto pnear = glm::mix(positions[i], positions[i + 4], lznear);
            auto pfar = glm::mix(positions[i], positions[i + 4], lzfar);

            positions[i] = pnear;
            positions[i + 4] = pfar;
        }

        float3 min = { std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max() };
        float3 max = { -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max() };

        for (auto i = 0; i < 8; ++i)
        {
            max = glm::max(float3(positions[i].xyz), max);
            min = glm::min(float3(positions[i].xyz), min);
        }

        return BoundingBox::MinMax(min, max);
    }

    BoundingBox GetInverseFrustumBounds(const float4x4& worldToLocal, const float4x4& inverseMatrix)
    {
        float4 positions[8];
        positions[0] = float4(-1, -1, PK_CLIPZ_NEAR, 1);
        positions[1] = float4(-1,  1, PK_CLIPZ_NEAR, 1);
        positions[2] = float4( 1,  1, PK_CLIPZ_NEAR, 1);
        positions[3] = float4( 1, -1, PK_CLIPZ_NEAR, 1);
        positions[4] = float4(-1, -1, PK_CLIPZ_FAR, 1);
        positions[5] = float4(-1,  1, PK_CLIPZ_FAR, 1);
        positions[6] = float4( 1,  1, PK_CLIPZ_FAR, 1);
        positions[7] = float4( 1, -1, PK_CLIPZ_FAR, 1);

        float3 min = { std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max() };
        float3 max = { -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max(), -std::numeric_limits<float>().max() };

        for (auto i = 0; i < 8; ++i)
        {
            positions[i] = inverseMatrix * positions[i];
            positions[i] /= positions[i].w;
            positions[i] = worldToLocal * positions[i];
            max = glm::max(float3(positions[i].xyz), max);
            min = glm::min(float3(positions[i].xyz), min);
        }

        return BoundingBox::MinMax(min, max);
    }
}