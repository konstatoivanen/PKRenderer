#pragma once
#include "Math.h"
#include "Bounds.h"

namespace PK::math
{
    // Produces Reverse Z
    template<typename T> matrix<T,4,4> orthographicFrustumBounding(const matrix<T,4,4>& worldToLocal, const matrix<T,4,4>& clipToView, const vector<T,3>& paddingLD, const vector<T,3>& paddingRU, T* outZNear, T* outZFar)
    {
        auto aabb = inverseFrustumToAABB(worldToLocal * clipToView);
        *outZNear = (aabb.min.z + paddingLD.z);
        *outZFar = (aabb.max.z + paddingRU.z);
        return orthographic(aabb.min.x + paddingLD.x, aabb.max.x + paddingRU.x, aabb.min.y + paddingLD.y, aabb.max.y + paddingRU.y, aabb.min.z + paddingLD.z, aabb.max.z + paddingRU.z) * worldToLocal;
    }

    template<typename T> vector<T,2> octawrap(const vector<T,2>& v) { return (static_cast<T>(1) - abs(v.yx())) * sign(v); }

    template<typename T> vector<T,2> octaencode(const vector<T,3>& n)
    {
        auto v = n;
        v /= (abs(v.x) + abs(v.y) + abs(v.z));
        v.xz = v.y >= static_cast<T>(0) ? v.xz : octawrap(v.xz);
        v.xz = v.xz * static_cast<T>(0.5) + static_cast<T>(0.5);
        return v.xz;
    }

    template<typename T> vector<T,3> triangleNormal(const T* p0, const T* p1, const T* p2, bool& outIsValid)
    {
        const T p10[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
        const T p20[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
        const T normalx = p10[1] * p20[2] - p10[2] * p20[1];
        const T normaly = p10[2] * p20[0] - p10[0] * p20[2];
        const T normalz = p10[0] * p20[1] - p10[1] * p20[0];
        const T area = sqrt(normalx * normalx + normaly * normaly + normalz * normalz);
        outIsValid = area != static_cast<T>(0);
        return vector<T,3>(normalx, normaly, normalz) / area;
    }

    template<typename T> vector<T,3> triangleNormal(const vector<T,3>& a, const vector<T,3>& b, const vector<T,3>& c)
    {
        bool isValid = false;
        return triangleNormal(&a.x, &b.x, &c.x, isValid);
    }

    template<typename T> bool intersectRects(const vector<T,4>& rect, const vector<T,4>& clipRect)
    {
        return (rect.x + rect.z) > clipRect.x && rect.x < (clipRect.x + clipRect.z) &&
               (rect.y + rect.w) > clipRect.y && rect.y < (clipRect.y + clipRect.w);
    }

    template<typename T> T cubicBezier(const T& p0, const T& p1, const T& cp0, const T& cp1, float t)
    {
        const auto u = 1.0f - t;
        const auto tt = t * t;
        const auto uu = u * u;
        return static_cast<T>((uu * u * p0) + (3.0f * uu * t * cp0) + (3.0f * u * tt * cp1) + (tt * t * p1));
    }

    template<typename T> vector<T,2> cubicBezier(const vector<T,2>& p0, const vector<T,2>& p1, const vector<T,2>& cp0, const vector<T,2>& cp1, float t)
    {
        return
        {
            cubicBezier(p0.x, p1.x, cp0.x, cp1.x, t),
            cubicBezier(p0.y, p1.y, cp0.y, cp1.y, t)
        };
    }

    template<typename T> vector<T,3> cubicBezier(const vector<T,3>& p0, const vector<T,3>& p1, const vector<T,3>& cp0, const vector<T,3>& cp1, float t)
    {
        return
        {
            cubicBezier(p0.x, p1.x, cp0.x, cp1.x, t),
            cubicBezier(p0.y, p1.y, cp0.y, cp1.y, t),
            cubicBezier(p0.z, p1.z, cp0.z, cp1.z, t)
        };
    }

    template<typename T> vector<T,4> cubicBezier(const vector<T,4>& p0, const vector<T,4>& p1, const vector<T,4>& cp0, const vector<T,4>& cp1, float t)
    {
        return
        {
            cubicBezier(p0.x, p1.x, cp0.x, cp1.x, t),
            cubicBezier(p0.y, p1.y, cp0.y, cp1.y, t),
            cubicBezier(p0.z, p1.z, cp0.z, cp1.z, t),
            cubicBezier(p0.w, p1.w, cp0.w, cp1.w, t)
        };
    }
}
