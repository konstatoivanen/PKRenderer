#pragma once
#include "Math.h"
#include "Bounds.h"

namespace PK::math
{
    template<typename T> uint64_t hash3x4(const matrix<T,3,4>& matrix, T precision)
    {
        #if PK_MATH_SIMD
        if constexpr (TIsSame<T, float>)
        {
            // Not very good but this is used to detect changes so one bit difference is ok.
            simd_f32vec4 inv = _mm_set1_ps(1.0f / precision);
            simd_u32vec4 r0 = _mm_cvtps_epi32(_mm_mul_ps(matrix[0].data, inv));
            simd_u32vec4 r1 = _mm_cvtps_epi32(_mm_mul_ps(matrix[1].data, inv));
            simd_u32vec4 r2 = _mm_cvtps_epi32(_mm_mul_ps(matrix[2].data, inv));
            simd_u32vec4 ac = _mm_add_epi64(_mm_xor_si128(r0, r1), r2);
            ulong2 u2(ac);

            uint64_t h = u2.x ^ (u2.y + 0x9E3779B185EBCA87ull);
            h ^= h >> 33ull;
            h *= 0xff51afd7ed558ccdULL;
            h ^= h >> 33ull;
            h *= 0xc4ceb9fe1a85ec53ULL;
            h ^= h >> 33ull;
            return h;
        }
        else
        #endif
        {
            uint64_t h = 14695981039346656037ULL;

            for (auto i = 0u; i < 3u; ++i)
            {
                h ^= (uint64_t)(matrix[i][0] / precision);
                h *= 1099511628211ULL;
                h ^= (uint64_t)(matrix[i][1] / precision);
                h *= 1099511628211ULL;
                h ^= (uint64_t)(matrix[i][2] / precision);
                h *= 1099511628211ULL;
                h ^= (uint64_t)(matrix[i][3] / precision);
                h *= 1099511628211ULL;
            }

            return h;
        }
    }

    // Produces Reverse Z
    template<typename T> matrix<T,4,4> orthographicFrustumBounding(const matrix<T,4,4>& worldToLocal, const matrix<T,4,4>& clipToView, const vector<T,3>& paddingLD, const vector<T,3>& paddingRU, T* outZNear, T* outZFar)
    {
        auto aabb = PK::Math::GetInverseFrustumBounds(worldToLocal * clipToView);
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
    
    void composeShadowCascadeMatrices(const ShadowCascadeCreateInfo info, float4x4* outMatrices);
}
