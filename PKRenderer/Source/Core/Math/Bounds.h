#pragma once
#include "Vector.h"

namespace PK::math
{
    template<typename T, int N>
    struct AABB
    {
        typedef vector<T,N> vec_type;
        vec_type min;
        vec_type max;

        constexpr AABB() = default;
        constexpr AABB(const AABB& m) = default;
        constexpr explicit AABB(T s) : min(s), max(s) {}
        constexpr AABB(const vec_type& mi, const vec_type& ma) : min(mi), max(ma) {}
        template<typename V0, typename V1> constexpr AABB(const vector<V0,N>& mi, const vector<V1,N>& ma) : min(mi), max(ma) {}

        constexpr AABB& operator=(const AABB& v) = default;
        template<typename U> constexpr AABB& operator=(U s) { min = static_cast<T>(s); max = static_cast<T>(s); return *this; }
        template<typename U> constexpr AABB& operator+=(U s) { min += static_cast<T>(s); max += static_cast<T>(s); return *this; }
        template<typename U> constexpr AABB& operator-=(U s) { min -= static_cast<T>(s); max -= static_cast<T>(s); return *this; }
        template<typename U> constexpr AABB& operator*=(U s) { min *= static_cast<T>(s); max *= static_cast<T>(s); return *this; }
        template<typename U> constexpr AABB& operator/=(U s) { min /= static_cast<T>(s); max /= static_cast<T>(s); return *this; }
        template<typename U> constexpr AABB& operator|=(U s) { min = math::min(min, s); max = math::max(max, s); return *this; }

        template<typename U> constexpr AABB& operator=(const vector<U, N>& v) { min = vec_type(v); max = vec_type(v); return *this; }
        template<typename U> constexpr AABB& operator+=(const vector<U,N>& v) { min += v; max += v; return *this; }
        template<typename U> constexpr AABB& operator-=(const vector<U,N>& v) { min -= v; max -= v; return *this; }
        template<typename U> constexpr AABB& operator*=(const vector<U,N>& v) { min *= v; max *= v; return *this; }
        template<typename U> constexpr AABB& operator/=(const vector<U,N>& v) { min /= v; max /= v; return *this; }
        template<typename U> constexpr AABB& operator|=(const vector<U,N>& v) { min = math::min(min, vec_type(v)); max = math::max(max, vec_type(v)); return *this; }
        
        template<typename U> constexpr AABB& operator=(const AABB<U,N>& b) { min = vec_type(b.min); max = vec_type(b.max); return *this; }
        template<typename U> constexpr AABB& operator+=(const AABB<U,N>& b) { min += b.min; max += b.max; return *this; }
        template<typename U> constexpr AABB& operator-=(const AABB<U,N>& b) { min -= b.min; max -= b.max; return *this; }
        template<typename U> constexpr AABB& operator*=(const AABB<U,N>& b) { min *= b.min; max *= b.max; return *this; }
        template<typename U> constexpr AABB& operator/=(const AABB<U,N>& b) { min /= b.min; max /= b.max; return *this; }
        template<typename U> constexpr AABB& operator|=(const AABB<U,N>& b) { min = math::min(min, vec_type(b.min)); max = math::max(max, vec_type(b.max)); return *this; }
        template<typename U> constexpr AABB& operator&=(const AABB<U,N>& b) { min = math::max(min, vec_type(b.min)); max = math::min(max, vec_type(b.max)); return *this; }

        constexpr vec_type center() const { return min + (max - min) * 0.5f; }
        constexpr vec_type extents() const { return (max - min) * 0.5f; }
        constexpr vec_type size() const { return max - min; }
    };

    template<typename T, int N> constexpr AABB<T,N> operator+(const AABB<T,N>& b) { return b; }
    template<typename T, int N> constexpr AABB<T,N> operator-(const AABB<T,N>& b) { return AABB<T,N>(-b.min, -b.max); }

    template<typename T, int N> constexpr AABB<T,N> operator+(const AABB<T,N>& b, T s) { return AABB<T,N>(b.min + s, b.max + s); }
    template<typename T, int N> constexpr AABB<T,N> operator-(const AABB<T,N>& b, T s) { return AABB<T,N>(b.min - s, b.max - s); }
    template<typename T, int N> constexpr AABB<T,N> operator*(const AABB<T,N>& b, T s) { return AABB<T,N>(b.min * s, b.max * s); }
    template<typename T, int N> constexpr AABB<T,N> operator/(const AABB<T,N>& b, T s) { return AABB<T,N>(b.min / s, b.max / s); }
    template<typename T, int N> constexpr AABB<T,N> operator|(const AABB<T,N>& b, T s) { return AABB<T,N>(math::min(b.min, s), math::max(b.max, s)); }

    template<typename T, int N> constexpr AABB<T,N> operator+(const AABB<T,N>& b, const vector<T,N>& v) { return AABB<T,N>(b.min + v, b.max + v); }
    template<typename T, int N> constexpr AABB<T,N> operator-(const AABB<T,N>& b, const vector<T,N>& v) { return AABB<T,N>(b.min - v, b.max - v); }
    template<typename T, int N> constexpr AABB<T,N> operator*(const AABB<T,N>& b, const vector<T,N>& v) { return AABB<T,N>(b.min * v, b.max * v); }
    template<typename T, int N> constexpr AABB<T,N> operator/(const AABB<T,N>& b, const vector<T,N>& v) { return AABB<T,N>(b.min / v, b.max / v); }
    template<typename T, int N> constexpr AABB<T,N> operator|(const AABB<T,N>& b, const vector<T,N>& v) { return AABB<T,N>(math::min(b.min, v), math::max(b.max, v)); }

    template<typename T, int N> constexpr AABB<T,N> operator+(const AABB<T,N>& a, const AABB<T,N>& b) { return AABB<T,N>(a.min + b.min, a.max + b.max); }
    template<typename T, int N> constexpr AABB<T,N> operator-(const AABB<T,N>& a, const AABB<T,N>& b) { return AABB<T,N>(a.min - b.min, a.max - b.max); }
    template<typename T, int N> constexpr AABB<T,N> operator*(const AABB<T,N>& a, const AABB<T,N>& b) { return AABB<T,N>(a.min * b.min, a.max * b.max); }
    template<typename T, int N> constexpr AABB<T,N> operator/(const AABB<T,N>& a, const AABB<T,N>& b) { return AABB<T,N>(a.min / b.min, a.max / b.max); }
    template<typename T, int N> constexpr AABB<T,N> operator|(const AABB<T,N>& a, const AABB<T,N>& b) { return AABB<T,N>(math::min(a.min, b.min), math::max(a.max, b.max)); }
    template<typename T, int N> constexpr AABB<T,N> operator&(const AABB<T,N>& a, const AABB<T,N>& b) { return AABB<T,N>(math::max(a.min, b.min), math::min(a.max, b.max)); }

    template<typename T, int N> constexpr bool operator==(const AABB<T,N>& a, const AABB<T,N>& b) { return (a.min == b.min) && (a.max == b.max); }
    template<typename T, int N> constexpr bool operator!=(const AABB<T,N>& a, const AABB<T,N>& b) { return (a.min != b.min) || (a.max != b.max); }
    
    template<typename T> constexpr AABB<T,3> mul(const matrix<T,4,4> mat, const AABB<T,3>& bb)
    {
        const matrix<T,3,3> absmat(abs(mat[0].xyz()), abs(mat[1].xyz()), abs(mat[2].xyz()));
        const auto c = (mat * vector<T,4>(bb.center(), static_cast<T>(1))).xyz();
        const auto e = absmat * bb.extents();
        return AABB<T,3>(c - e, c + e);
    }

    template<typename T> constexpr AABB<T,3> mul(const matrix<T,3,4> mat, const AABB<T,3>& bb)
    {
        const matrix<T,3,3> absmat(abs(mat[0].xyz()), abs(mat[1].xyz()), abs(mat[2].xyz()));
        const auto c = vector<T,4>(bb.center(), static_cast<T>(1)) * mat;
        const auto e = absmat * bb.extents();
        return AABB<T,3>(c - e, c + e);
    }

    template<typename T> constexpr T distanceToPlaneMin(const AABB<T,3>& bb, const vector<T,4>& plane)
    {
        auto bx = plane.x < static_cast<T>(0) ? bb.max.x : bb.min.x;
        auto by = plane.y < static_cast<T>(0) ? bb.max.y : bb.min.y;
        auto bz = plane.z < static_cast<T>(0) ? bb.max.z : bb.min.z;
        return plane.x * bx + plane.y * by + plane.z * bz + plane.w;
    }

    template<typename T> constexpr T distanceToPlaneMax(const AABB<T,3>& bb, const vector<T,4>& plane)
    {
        auto bx = plane.x > static_cast<T>(0) ? bb.max.x : bb.min.x;
        auto by = plane.y > static_cast<T>(0) ? bb.max.y : bb.min.y;
        auto bz = plane.z > static_cast<T>(0) ? bb.max.z : bb.min.z;
        return plane.x * bx + plane.y * by + plane.z * bz + plane.w;
    }

    template<typename T, int N> constexpr bool intersects(const AABB<T,N>& bb, const vector<T,N>& point) 
    {
        return all(bb.min <= point) && all(bb.max >= point); 
    }
    
    template<typename T, int N> constexpr bool intersects(const AABB<T,N>& aa, const AABB<T,N>& bb) 
    {
        return all(aa.min <= bb.max) && all(aa.max >= bb.min); 
    }
    
    template<typename T> constexpr bool intersectsPlane(const AABB<T,3>& bb, const vector<T,4>& p) 
    { 
        const auto c = bb.center();
        const auto e = bb.extents();
        const auto r = e.x * abs(p.x) + e.y * Abs(p.y) + e.z * abs(p.z);
        const auto s = (p.x * c.x + p.y * c.y + p.z * c.z) - p.w;
        return abs(s) <= r;
    }

    template<typename T> constexpr bool intersectsSphere(const AABB<T, 3>& bb, const vector<T,4>& s)
    {
        const auto d = clamp(s.xyz(), bb.min, bb.max) - s.xyz();
        return dot(d, d) <= s.w * s.w;
    }

    template<typename T> constexpr bool intersectsConvex(const AABB<T,3>& aabb, const float4* planes, uint32_t count)
    {
        for (auto i = 0u; i < count; ++i)
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

    template<typename T, int N> constexpr uint32_t longestAxis(const AABB<T, N>& bb)
    {
        auto extents = bb.extents();
        auto length = extents[0];
        auto axis = 0u;

        for (auto i = 1u; i < N; ++i)
        {
            axis = lerp(axis, i, extents[i] > length);
            length = lerp(length, extents[i], extents[i] > length);
        }

        return axis;
    }

    template<typename T, int N> constexpr uint32_t shortestAxis(const AABB<T, N>& bb)
    {
        auto extents = bb.extents();
        auto length = extents[0];
        auto axis = 0u;

        for (auto i = 1u; i < N; ++i)
        {
            axis = lerp(axis, i, extents[i] < length);
            length = lerp(length, extents[i], extents[i] < length);
        }

        return axis;
    }

    template<typename T, int N> constexpr AABB<T,N> split(const AABB<T,N>& bounds, uint32_t axis, AABB<T,N>* out0, AABB<T,N>* out1)
    {
        *out0 = bounds;
        *out1 = bounds;
        out0->max[axis] = bounds.min[axis] + (bounds.max[axis] - bounds.min[axis]) * 0.5f;
        out1->min[axis] = bounds.min[axis] + (bounds.max[axis] - bounds.min[axis]) * 0.5f;
    }

    template<typename T, int N> constexpr AABB<T,N> minmaxToAABB(const vector<T,N>& min, const vector<T,N>& max) { return AABB<T,N>(min, max); }

    template<typename T, int N> constexpr AABB<T,N> centerExtentsToAABB(const vector<T,N>& center, const vector<T,N>& extents) { return AABB<T,N>(center - extents, center + extents); }

    template<typename T, int N> constexpr AABB<T,N> pointsToAABB(const vector<T,N>* points, uint32_t count)
    {
        AABB<T,N> result{};

        if (count > 0u)
        {
            result.min = points[0u];
            result.max = points[0u];

            for (auto i = 1u; i < count; ++i)
            {
                result |= points[i];
            }
        }

        return result;
    }
    
    template<typename T> constexpr vector<T,4> pointsToSphere(const vector<T,3>* points, uint32_t count)
    {
        size_t pmin[3] = { 0, 0, 0 };
        size_t pmax[3] = { 0, 0, 0 };

        for (size_t i = 0u; i < count; ++i)
        {
            const auto& p = points[i];

            for (int axis = 0; axis < 3; ++axis)
            {
                pmin[axis] = (p[axis] < points[pmin[axis]][axis]) ? i : pmin[axis];
                pmax[axis] = (p[axis] > points[pmax[axis]][axis]) ? i : pmax[axis];
            }
        }

        // find the pair of points with largest distance
        T paxisd2 = 0;
        int paxis = 0;

        for (int axis = 0; axis < 3; ++axis)
        {
            const auto& p1 = points[pmin[axis]];
            const auto& p2 = points[pmax[axis]];
            auto d2 = (p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) + (p2[2] - p1[2]) * (p2[2] - p1[2]);

            if (d2 > paxisd2)
            {
                paxisd2 = d2;
                paxis = axis;
            }
        }

        // use the longest segment as the initial sphere diameter
        const auto& p1 = points[pmin[paxis]];
        const auto& p2 = points[pmax[paxis]];
        T center[3] = { (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2, (p1[2] + p2[2]) / 2 };
        T radius = sqrt(paxisd2) / 2;

        // iteratively adjust the sphere up until all points fit
        for (size_t i = 0; i < count; ++i)
        {
            const auto& p = points[i];
            float d2 = (p[0] - center[0]) * (p[0] - center[0]) + (p[1] - center[1]) * (p[1] - center[1]) + (p[2] - center[2]) * (p[2] - center[2]);

            if (d2 > radius * radius)
            {
                auto d = sqrt(d2);
                auto k = static_cast<T>(0.5) + (radius / d) / 2;
                center[0] = center[0] * k + p[0] * (1 - k);
                center[1] = center[1] * k + p[1] * (1 - k);
                center[2] = center[2] * k + p[2] * (1 - k);
                radius = (radius + d) / 2;
            }
        }

        return vector<T,4>(center[0], center[1], center[2], radius);
    }

    template<typename T> constexpr AABB<T,3> inverseFrustumToAABB(const matrix<T,4,4>& inverseMatrix)
    {
        vector<T,4> positions[8];
        positions[0] = inverseMatrix * vector<T,4>(-1,-1, PK_CLIPZ_NEAR, 1);
        positions[1] = inverseMatrix * vector<T,4>(-1,+1, PK_CLIPZ_NEAR, 1);
        positions[2] = inverseMatrix * vector<T,4>(+1,+1, PK_CLIPZ_NEAR, 1);
        positions[3] = inverseMatrix * vector<T,4>(+1,-1, PK_CLIPZ_NEAR, 1);
        positions[4] = inverseMatrix * vector<T,4>(-1,-1, PK_CLIPZ_FAR, 1);
        positions[5] = inverseMatrix * vector<T,4>(-1,+1, PK_CLIPZ_FAR, 1);
        positions[6] = inverseMatrix * vector<T,4>(+1,+1, PK_CLIPZ_FAR, 1);
        positions[7] = inverseMatrix * vector<T,4>(+1,-1, PK_CLIPZ_FAR, 1);
        auto result = AABB<T, 3>(+vector<T,3>(FLT_MAX, FLT_MAX, FLT_MAX), -vector<T,3>(FLT_MAX, FLT_MAX, FLT_MAX));

        for (auto i = 0; i < 8; ++i)
        {
            positions[i] /= positions[i].w;
            result |= positions[i].xyz();
        }

        return result;
    }

    template<typename T> constexpr AABB<T,3> inverseFrustumToAABB(const matrix<T,4,4>& inverseMatrix, T lznear, T lzfar)
    {
        vector<T, 4> positions[8];
        positions[0] = inverseMatrix * vector<T, 4>(-1, -1, PK_CLIPZ_NEAR, 1);
        positions[1] = inverseMatrix * vector<T, 4>(-1, +1, PK_CLIPZ_NEAR, 1);
        positions[2] = inverseMatrix * vector<T, 4>(+1, +1, PK_CLIPZ_NEAR, 1);
        positions[3] = inverseMatrix * vector<T, 4>(+1, -1, PK_CLIPZ_NEAR, 1);
        positions[4] = inverseMatrix * vector<T, 4>(-1, -1, PK_CLIPZ_FAR, 1);
        positions[5] = inverseMatrix * vector<T, 4>(-1, +1, PK_CLIPZ_FAR, 1);
        positions[6] = inverseMatrix * vector<T, 4>(+1, +1, PK_CLIPZ_FAR, 1);
        positions[7] = inverseMatrix * vector<T, 4>(+1, -1, PK_CLIPZ_FAR, 1);
        auto result = AABB<T,3>(+vector<T,3>(FLT_MAX, FLT_MAX, FLT_MAX), -vector<T,3>(FLT_MAX, FLT_MAX, FLT_MAX));

        for (auto i = 0u; i < 4u; ++i)
        {
            positions[i] /= positions[i].w;
            positions[i + 4u] /= positions[i + 4u].w;
            const auto pnear = math::lerp(positions[i], positions[i + 4u], lznear);
            const auto pfar = math::lerp(positions[i], positions[i + 4u], lzfar);
            result |= pnear.xyz();
            result |= pfar.xyz();
        }

        return result;
    }

    template<typename T> constexpr AABB<T,3> inverseFrustumToAABB(const matrix<T, 4, 4>& worldToLocal, const matrix<T, 4, 4>& inverseMatrix)
    {
        vector<T,4> positions[8];
        positions[0] = inverseMatrix * vector<T,4>(-1, -1, PK_CLIPZ_NEAR, 1);
        positions[1] = inverseMatrix * vector<T,4>(-1, +1, PK_CLIPZ_NEAR, 1);
        positions[2] = inverseMatrix * vector<T,4>(+1, +1, PK_CLIPZ_NEAR, 1);
        positions[3] = inverseMatrix * vector<T,4>(+1, -1, PK_CLIPZ_NEAR, 1);
        positions[4] = inverseMatrix * vector<T,4>(-1, -1, PK_CLIPZ_FAR, 1);
        positions[5] = inverseMatrix * vector<T,4>(-1, +1, PK_CLIPZ_FAR, 1);
        positions[6] = inverseMatrix * vector<T,4>(+1, +1, PK_CLIPZ_FAR, 1);
        positions[7] = inverseMatrix * vector<T,4>(+1, -1, PK_CLIPZ_FAR, 1);
        auto result = AABB<T,3>(+vector<T,3>(FLT_MAX, FLT_MAX, FLT_MAX), -vector<T,3>(FLT_MAX, FLT_MAX, FLT_MAX));

        for (auto i = 0; i < 8; ++i)
        {
            positions[i] = inverseMatrix * positions[i];
            positions[i] /= positions[i].w;
            positions[i] = worldToLocal * positions[i];
            result |= positions[i].xyz();
        }

        return result;
    }


    template<typename T, int N>
    struct convex
    {
        vector<T,4> planes[N];
        constexpr convex() = default;
        constexpr convex(const convex& m) = default;
        constexpr const vector<T,4>& operator[](size_t i) const { return planes[i]; }
        constexpr vector<T,4>& operator[](size_t i) { return planes[i]; }
        constexpr const vector<T,4>* array_ptr() const { return planes; }
        constexpr vector<T,4>* array_ptr() { return planes; }
    };

    template<typename T> constexpr vector<T, 4> normalizePlane(const vector<T, 4>& p) { return p * rsqrt(p.x * p.x + p.y * p.y + p.z * p.z); }

    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
    // DirectX convention
    template<bool normalize, typename T> convex<T,6> frustumConvex(const matrix<T,4,4>& viewToClip)
    {
        convex<T,6> planes;
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
        // Far clipping plane
        planes[4].x = viewToClip[0][2];
        planes[4].y = viewToClip[1][2];
        planes[4].z = viewToClip[2][2];
        planes[4].w = viewToClip[3][2];
        // Near clipping plane
        planes[5].x = viewToClip[0][3] - viewToClip[0][2];
        planes[5].y = viewToClip[1][3] - viewToClip[1][2];
        planes[5].z = viewToClip[2][3] - viewToClip[2][2];
        planes[5].w = viewToClip[3][3] - viewToClip[3][2];

        // Normalize the plane equations, if requested
        if constexpr (normalize)
        {
            planes[0] = normalizePlane(planes[0]);
            planes[1] = normalizePlane(planes[1]);
            planes[2] = normalizePlane(planes[2]);
            planes[3] = normalizePlane(planes[3]);
            planes[4] = normalizePlane(planes[4]);
            planes[5] = normalizePlane(planes[5]);
        }

        return planes;
    }

    template<typename T> vector<T,4> nearPlane(const matrix<T,4,4>& viewToClip)
    {
        return normalizePlane(vector<T,4>(viewToClip[0][3] - viewToClip[0][2], viewToClip[1][3] - viewToClip[1][2], viewToClip[2][3] - viewToClip[2][2], viewToClip[3][3] - viewToClip[3][2]));
    }

    template<typename T> vector<T,4> mulplanar(const matrix<T,4,4>& matrix, const vector<T,4>& plane)
    {
        const auto direction = (matrix * vector<T,4>(plane.xyz, static_cast<T>(0))).xyz();
        const auto offset = (matrix * vector<T,4>(plane.xyz * plane.w, static_cast<T>(1))).xyz();
        return vector<T,4>(direction, -dot(direction, offset));
    }

    template<typename T> vector<T,4> mulplanar(const matrix<T,3,4>& matrix, const vector<T,4>& plane)
    {
        const auto direction = vector<T,4>(plane.xyz, static_cast<T>(0)) * matrix;
        const auto offset = vector<T,4>(plane.xyz * plane.w, static_cast<T>(1)) * matrix;
        return vector<T,4>(direction, -dot(direction, offset));
    }

    template<typename T> T distanceToPoint(const vector<T,4>& plane, const vector<T,3>& point)
    {
        return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
    }

    template<typename T> vector<T,3> triplanarIntersection(const vector<T,4>& p1, const vector<T,4>& p2, const vector<T,4>& p3)
    {
        vector<T,3> n1 = p1.xyz, n2 = p2.xyz, n3 = p3.xyz;
        return ((-p1.w * cross(n2, n3)) + (-p2.w * cross(n3, n1)) + (-p3.w * cross(n1, n2))) / (dot(n1, math::cross(n2, n3)));
    }

    template<typename T> T distanceToExtents(const vector<T,3>& point, const vector<T,3>& extents)
    {
        const auto q = abs(point) - extents;
        return length(max(q, static_cast<T>(0))) + min(cmax(q), static_cast<T>(0));
    }
}
