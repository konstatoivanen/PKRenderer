#pragma once
#include "Vector.h"

namespace PK::math
{
    template<typename T> constexpr T farClip(const matrix<T,4,4>& clip) { return -clip[3][2] / (clip[2][2] - PK_CLIPZ_FAR); }
    template<typename T> constexpr T nearClip(const matrix<T,4,4>& clip) { return -clip[3][2] / (clip[2][2] - PK_CLIPZ_NEAR); }
    template<typename T> constexpr T sizeOnScreen(T depth, T sizePerDepth, T radius) { return radius / (sizePerDepth * depth); }
    template<typename T> constexpr T clipDepthExp(T viewz, const vector<T,3>& params) { return log2(viewz * params.x + params.y) * params.z; }
    template<typename T> constexpr T viewDepthExp(T clipz, const vector<T,3>& params) { return (exp2(clipz / params.z) - params.y) / params.x; }

    template<typename T> vector<T,3> exponentialZParams01(T znear, T zfar, T distribution)
    {
        const auto o = (zfar - znear * exp2(rcp(distribution))) / (zfar - znear);
        const auto b = (static_cast<T>(1) - o) / znear;
        return vector<T,3>(b, o, distribution);
    }

    template<typename T> vector<T,3> exponentialZParams(T znear, T zfar, T distribution, uint32_t size)
    {
        const auto o = (zfar - znear * exp2((size - static_cast<T>(1)) / distribution)) / (zfar - znear);
        const auto b = (static_cast<T>(1) - o) / znear;
        return vector<T,3>(b, o, distribution);
    }

    template<typename T, uint32_t N> void cascadeDepths(T znear, T zfar, T distribution, T* cascades)
    {
        static_assert(N > 2u, "Cascade depths cannot be generated for less than count 3");
        cascades[0] = znear;
        cascades[N - 1u] = zfar;
        const auto zparams = exponentialZParams01(znear, zfar, distribution);

        for (auto i = 1u; i < N - 1u; ++i)
        {
            cascades[i] = viewDepthExp(i / static_cast<T>(N - 1u), zparams);
        }
    }

    template<typename T, uint32_t N> void cascadeDepths(T znear, T zfar, T distribution, T* cascades, const vector<T,3>& zAlignParams)
    {
        cascadeDepths<T,N>(znear, zfar, distribution, cascades);

        // Snap z ranges to tile indices to make shader branching more coherent
        for (auto i = 1u; i < (N - 1u); ++i)
        {
            cascades[i] = viewDepthExp(round(clipDepthExp(cascades[i], zAlignParams)), zAlignParams);
        }
    }

    template<typename T> vector<T,4> cascadeDepths4(T znear, T zfar, T distribution, const vector<T,3>& zAlignParams)
    {
        // Ignore first plane as it is just the near plane and we are more interested in the final clipping plane.
        T cascades[5]{};
        cascadeDepths<T,5>(znear, zfar, distribution, cascades, zAlignParams);
        return vector<T,4>(cascades[1u], cascades[2u], cascades[3u], cascades[4u]);
    }

    // Produces Reverse Z
    template<typename T> matrix<T,4,4> perspective(T fov, T aspect, T zNear, T zFar)
    {
        const auto tanHalfFovy = tan(fov * static_cast<T>(PK_FLOAT_DEG2RAD / 2.0f));
        matrix<T,4,4> m(static_cast<T>(0));
        m[0][0] = rcp(aspect * tanHalfFovy);
        m[1][1] = rcp(tanHalfFovy);
        m[2][2] = -zNear / (zFar - zNear);
        m[2][3] = static_cast<T>(1);
        m[3][2] = (zNear * zFar) / (zFar - zNear);
        return m;
    }

    // Produces Reverse Z
    template<typename T> matrix<T,4,4> perspectiveOffset(T left, T right, T bottom, T top, T fovy, T aspect, T zNear, T zFar)
    {
        const auto tanHalfFovy = tan(fovy * static_cast<T>(PK_FLOAT_DEG2RAD / 2.0f));
        const auto rcpRL = rcp(right - left);
        const auto rcpTB = rcp(top - bottom);
        const auto rcpFN = rcp(zFar - zNear);
        matrix<T,4,4> m(static_cast<T>(0));
        m[0][0] = (static_cast<T>(2) * rcpRL) / (aspect * tanHalfFovy);
        m[1][1] = (static_cast<T>(2) * rcpTB) / tanHalfFovy;
        m[2][0] = -(right + left) * rcpRL;
        m[2][1] = -(top + bottom) * rcpTB;
        m[2][2] = -zNear * rcpFN;
        m[2][3] = static_cast<T>(1);
        m[3][2] = zFar * zNear * rcpFN;
        return m;
    }

    // Produces Reverse Z
    template<typename T> matrix<T,4,4> perspectiveSubDivision(uint32_t index, const int3& gridSize, T fovy, T aspect, T znear, T zfar)
    {
        int3 coord(index % gridSize.x, (index / gridSize.x) % gridSize.y, index / (gridSize.x * gridSize.y));
        const T ix = static_cast<T>(2) / gridSize.x;
        const T iy = static_cast<T>(2) / gridSize.y;
        const T iz = static_cast<T>(1) / gridSize.z;
        const T x = -static_cast<T>(1) + (static_cast<T>(2) * static_cast<T>(coord.x)) / gridSize.x;
        const T y = -static_cast<T>(1) + (static_cast<T>(2) * static_cast<T>(coord.y)) / gridSize.y;
        const T z = static_cast<T>(coord.z) / gridSize.z;
        const T zrange = zfar - znear;
        return composePerspectiveOffset(x, x + ix, y, y + iy, fovy, aspect, znear + zrange * z, znear + zrange * (z + iz));
    }

    // Produces Reverse Z
    template<typename T> matrix<T,4,4> perspectiveJittered(const matrix<T,4,4>& m, const vector<T,2>& jitter)
    {
        matrix<T,4,4> r = m;
        r[2][0] += jitter.x;
        r[2][1] += jitter.y;
        return r;
    }

    // Produces Reverse Z
    template<typename T> matrix<T,4,4> orthographic(T left, T right, T bottom, T top, T zNear, T zFar)
    {
        matrix<T,4,4> m(static_cast<T>(1));
        m[0][0] = static_cast<T>(2) / (right - left);
        m[1][1] = static_cast<T>(2) / (top - bottom);
        m[2][2] = -rcp(zFar - zNear);
        m[3][0] = -(right + left) / (right - left);
        m[3][1] = -(top + bottom) / (top - bottom);
        m[3][2] = zFar / (zFar - zNear);
        return m;
    }
}
