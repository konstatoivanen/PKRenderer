#pragma once
#include "Forward.h"

#if PK_MATH_SIMD

namespace PK::math
{
    #if PK_MATH_SIMD_SSE4_1  
    #define simd_mullo _mm_mullo_epi32
    #define simd_extract_low_epi64(v) _mm_extract_epi64(v,0)
    #define simd_extract_high_epi64(v) _mm_extract_epi64(v,1)
    #else
    inline __m128i simd_mullo(__m128i a, __m128i b)
    {
        auto v0 = _mm_mul_epu32(a, b);
        auto v1 = _mm_mul_epu32(_mm_srli_si128(a, 4), _mm_srli_si128(b, 4));
        auto v2 = _mm_shuffle_epi32(v0, _MM_SHUFFLE(0, 0, 2, 0));
        auto v3 = _mm_shuffle_epi32(v1, _MM_SHUFFLE(0, 0, 2, 0));
        return _mm_unpacklo_epi32(v2, v3);
    }
    #define simd_extract_low_epi64(v) _mm_cvtsi128_si64(v)
    #define simd_extract_high_epi64(v) _mm_cvtsi128_si64(_mm_srli_si128(v, 8))
    #endif

    inline simd_i32vec4 simd_cast_int(simd_i32vec4 v) { return v; }
    inline simd_i32vec4 simd_cast_int(simd_f32vec4 v) { return _mm_castps_si128(v); }
    inline simd_f32vec4 simd_cast_float(simd_f32vec4 v) { return v; }
    inline simd_f32vec4 simd_cast_float(simd_i32vec4 v) { return _mm_castsi128_ps(v); }

    inline simd_i32vec4 simd_cvt_int(simd_i32vec4 v) { return v; }
    inline simd_i32vec4 simd_cvt_int(simd_f32vec4 v) { return _mm_cvtps_epi32(v); }
    inline simd_f32vec4 simd_cvt_float(simd_f32vec4 v) { return v; }
    inline simd_f32vec4 simd_cvt_float(simd_i32vec4 v) { return _mm_cvtepi32_ps(v); }

    template<typename T, int N>
    inline uint64_t simd_hash(const T* v)
    {
        const auto fnv_prime = _mm_set1_epi32(16777619);
        auto hash = _mm_set1_epi32(2166136261);

        for (auto i = 0u; i < N; ++i)
        {
            auto data = simd_cast_int(v[i]);
            hash = _mm_xor_si128(hash, data);
            hash = simd_mullo(hash, fnv_prime);
        }

        auto h = static_cast<uint64_t>(simd_extract_low_epi64(hash));
        h *= 1099511628211ull;
        h ^= static_cast<uint64_t>(simd_extract_high_epi64(hash));
        h *= 1099511628211ull;
        return h;
    }

    template<typename T, int N>
    inline uint64_t simd_hash_quantized(const T* v, float precision)
    {
        const auto inv = _mm_set1_ps(1.0f / precision);
        const auto fnv_prime = _mm_set1_epi32(16777619u);
        auto hash = _mm_set1_epi32(2166136261u);

        for (auto i = 0u; i < N; ++i)
        {
            auto v0 = simd_cvt_float(v[i]);
            auto v1 = _mm_mul_ps(v0, inv);
            auto v2 = _mm_cvtps_epi32(v1);
            hash = _mm_xor_si128(hash, v2);
            hash = simd_mullo(hash, fnv_prime);
        }

        auto h = static_cast<uint64_t>(simd_extract_low_epi64(hash));
        h *= 1099511628211ull;
        h ^= static_cast<uint64_t>(simd_extract_high_epi64(hash));
        h *= 1099511628211ull;
        return h;
    }

    inline simd_f32vec4 simd_dot_4(simd_f32vec4 v1, simd_f32vec4 v2)
    {
        #if PK_MATH_SIMD_SSE4_1
        return _mm_dp_ps(v1, v2, 0xff);
        #elif PK_MATH_SIMD_SSE3
        const auto mul0 = _mm_mul_ps(v1, v2);
        const auto hadd0 = _mm_hadd_ps(mul0, mul0);
        const auto hadd1 = _mm_hadd_ps(hadd0, hadd0);
        return hadd1;
        #else
        const auto mul0 = _mm_mul_ps(v1, v2);
        const auto swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
        const auto add0 = _mm_add_ps(mul0, swp0);
        const auto swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(0, 1, 2, 3));
        const auto add1 = _mm_add_ps(add0, swp1);
        return add1;
        #endif
    }

    template<> inline float4 operator-(const float4& v) { return float4(_mm_xor_ps(v.data, _mm_set1_ps(-0.0f))); }
    /*
    * Scalar operators disabled. unaligned load into vector regs + vector op was slower than piece wise scalar op.
    template<> inline float4 operator+(const float4& v, float s) { return float4(_mm_add_ps(v.data, _mm_set1_ps(s))); }
    template<> inline float4 operator-(const float4& v, float s) { return float4(_mm_sub_ps(v.data, _mm_set1_ps(s))); }
    template<> inline float4 operator*(const float4& v, float s) { return float4(_mm_mul_ps(v.data, _mm_set1_ps(s))); }
    template<> inline float4 operator/(const float4& v, float s) { return float4(_mm_div_ps(v.data, _mm_set1_ps(s))); }
    template<> inline float4 operator+(float s, const float4& v) { return float4(_mm_add_ps(_mm_set1_ps(s), v.data)); }
    template<> inline float4 operator-(float s, const float4& v) { return float4(_mm_sub_ps(_mm_set1_ps(s), v.data)); }
    template<> inline float4 operator*(float s, const float4& v) { return float4(_mm_mul_ps(_mm_set1_ps(s), v.data)); }
    template<> inline float4 operator/(float s, const float4& v) { return float4(_mm_div_ps(_mm_set1_ps(s), v.data)); }
    */
    template<> inline float4 operator+(const float4& a, const float4& b) { return float4(_mm_add_ps(a.data, b.data)); }
    template<> inline float4 operator-(const float4& a, const float4& b) { return float4(_mm_sub_ps(a.data, b.data)); }
    template<> inline float4 operator*(const float4& a, const float4& b) { return float4(_mm_mul_ps(a.data, b.data)); }
    template<> inline float4 operator/(const float4& a, const float4& b) { return float4(_mm_div_ps(a.data, b.data)); }
    
    template<> inline int4 operator-(const int4& v) { return int4(_mm_sub_epi32(_mm_setzero_si128(), v.data)); }
    template<> inline int4 operator~(const int4& v) { return int4(_mm_xor_si128(v.data, _mm_set1_epi32(-1))); }
    /*
    * Scalar operators disabled. unaligned load into vector regs + vector op was slower than piece wise scalar op.
    template<> inline int4 operator+(const int4& v, int32_t s) { return int4(_mm_add_epi32(v.data, _mm_set1_epi32(s))); }
    template<> inline int4 operator-(const int4& v, int32_t s) { return int4(_mm_sub_epi32(v.data, _mm_set1_epi32(s))); }
    template<> inline int4 operator*(const int4& v, int32_t s) { return int4(simd_mullo(v.data, _mm_set1_epi32(s))); }
    template<> inline int4 operator&(const int4& v, int32_t s) { return int4(_mm_and_si128(v.data, _mm_set1_epi32(s))); }
    template<> inline int4 operator|(const int4& v, int32_t s) { return int4(_mm_or_si128(v.data, _mm_set1_epi32(s))); }
    template<> inline int4 operator^(const int4& v, int32_t s) { return int4(_mm_xor_si128(v.data, _mm_set1_epi32(s))); }
    template<> inline int4 operator+(int32_t s, const int4& v) { return int4(_mm_add_epi32(_mm_set1_epi32(s), v.data)); }
    template<> inline int4 operator-(int32_t s, const int4& v) { return int4(_mm_sub_epi32(_mm_set1_epi32(s), v.data)); }
    template<> inline int4 operator*(int32_t s, const int4& v) { return int4(simd_mullo(_mm_set1_epi32(s), v.data)); }
    template<> inline int4 operator&(int32_t s, const int4& v) { return int4(_mm_and_si128(_mm_set1_epi32(s), v.data)); }
    template<> inline int4 operator|(int32_t s, const int4& v) { return int4(_mm_or_si128(_mm_set1_epi32(s), v.data)); }
    template<> inline int4 operator^(int32_t s, const int4& v) { return int4(_mm_xor_si128(_mm_set1_epi32(s), v.data)); }
    */
    template<> inline int4 operator+(const int4& a, const int4& b) { return int4(_mm_add_epi32(a.data, b.data)); }
    template<> inline int4 operator-(const int4& a, const int4& b) { return int4(_mm_sub_epi32(a.data, b.data)); }
    template<> inline int4 operator*(const int4& a, const int4& b) { return int4(simd_mullo(a.data, b.data)); }
    template<> inline int4 operator&(const int4& a, const int4& b) { return int4(_mm_and_si128(a.data, b.data)); }
    template<> inline int4 operator|(const int4& a, const int4& b) { return int4(_mm_or_si128(a.data, b.data)); }
    template<> inline int4 operator^(const int4& a, const int4& b) { return int4(_mm_xor_si128(a.data, b.data)); }

    template<> inline uint4 operator-(const uint4& v) { return uint4(_mm_sub_epi32(_mm_setzero_si128(), v.data)); }
    template<> inline uint4 operator~(const uint4& v) { return uint4(_mm_xor_si128(v.data, _mm_set1_epi32(-1))); }
    /*
    * Scalar operators disabled. unaligned load into vector regs + vector op was slower than piece wise scalar op.
    template<> inline uint4 operator+(const uint4& v, uint32_t s) { return uint4(_mm_add_epi32(v.data, _mm_set1_epi32(s))); }
    template<> inline uint4 operator-(const uint4& v, uint32_t s) { return uint4(_mm_sub_epi32(v.data, _mm_set1_epi32(s))); }
    template<> inline uint4 operator*(const uint4& v, uint32_t s) { return uint4(simd_mullo(v.data, _mm_set1_epi32(s))); }
    template<> inline uint4 operator&(const uint4& v, uint32_t s) { return uint4(_mm_and_si128(v.data, _mm_set1_epi32(s))); }
    template<> inline uint4 operator|(const uint4& v, uint32_t s) { return uint4(_mm_or_si128(v.data, _mm_set1_epi32(s))); }
    template<> inline uint4 operator^(const uint4& v, uint32_t s) { return uint4(_mm_xor_si128(v.data, _mm_set1_epi32(s))); }
    template<> inline uint4 operator+(uint32_t s, const uint4& v) { return uint4(_mm_add_epi32(_mm_set1_epi32(s), v.data)); }
    template<> inline uint4 operator-(uint32_t s, const uint4& v) { return uint4(_mm_sub_epi32(_mm_set1_epi32(s), v.data)); }
    template<> inline uint4 operator*(uint32_t s, const uint4& v) { return uint4(simd_mullo(_mm_set1_epi32(s), v.data)); }
    template<> inline uint4 operator&(uint32_t s, const uint4& v) { return uint4(_mm_and_si128(_mm_set1_epi32(s), v.data)); }
    template<> inline uint4 operator|(uint32_t s, const uint4& v) { return uint4(_mm_or_si128(_mm_set1_epi32(s), v.data)); }
    template<> inline uint4 operator^(uint32_t s, const uint4& v) { return uint4(_mm_xor_si128(_mm_set1_epi32(s), v.data)); }
    */
    template<> inline uint4 operator+(const uint4& a, const uint4& b) { return uint4(_mm_add_epi32(a.data, b.data)); }
    template<> inline uint4 operator-(const uint4& a, const uint4& b) { return uint4(_mm_sub_epi32(a.data, b.data)); }
    template<> inline uint4 operator*(const uint4& a, const uint4& b) { return uint4(simd_mullo(a.data, b.data)); }
    template<> inline uint4 operator&(const uint4& a, const uint4& b) { return uint4(_mm_and_si128(a.data, b.data)); }
    template<> inline uint4 operator|(const uint4& a, const uint4& b) { return uint4(_mm_or_si128(a.data, b.data)); }
    template<> inline uint4 operator^(const uint4& a, const uint4& b) { return uint4(_mm_xor_si128(a.data, b.data)); }

    template<> inline float4 operator*(const float4x4& m, const float4& v)
    {
        auto vd = v.data;
        auto m0 = m[0].data;
        auto m1 = m[1].data;
        auto m2 = m[2].data;
        auto m3 = m[3].data;
        m0 = _mm_mul_ps(m0, _mm_shuffle_ps(vd, vd, _MM_SHUFFLE(0, 0, 0, 0)));
        m1 = _mm_mul_ps(m1, _mm_shuffle_ps(vd, vd, _MM_SHUFFLE(1, 1, 1, 1)));
        m0 = _mm_add_ps(m0, m1);
        m2 = _mm_mul_ps(m2, _mm_shuffle_ps(vd, vd, _MM_SHUFFLE(2, 2, 2, 2)));
        m3 = _mm_mul_ps(m3, _mm_shuffle_ps(vd, vd, _MM_SHUFFLE(3, 3, 3, 3)));
        m2 = _mm_add_ps(m2, m3);
        m0 = _mm_add_ps(m0, m2);
        return float4(m0);
    }

    template<> inline float4 operator*(const float4& v, const float4x4& m)
    {
        auto vd = v.data;
        auto m0 = m[0].data;
        auto m1 = m[1].data;
        auto m2 = m[2].data;
        auto m3 = m[3].data;
        m0 = _mm_mul_ps(vd, m0);
        m1 = _mm_mul_ps(vd, m1);
        m2 = _mm_mul_ps(vd, m2);
        m3 = _mm_mul_ps(vd, m3);
        #if PK_MATH_SIMD_SSE3
        auto h0 = _mm_hadd_ps(m0, m1);
        auto h1 = _mm_hadd_ps(m2, m3);
        return float4(_mm_hadd_ps(h0, h1));
        #else
        auto u0 = _mm_unpacklo_ps(m0, m1);
        auto u1 = _mm_unpackhi_ps(m0, m1);
        auto a0 = _mm_add_ps(u0, u1);
        auto u2 = _mm_unpacklo_ps(m2, m3);
        auto u3 = _mm_unpackhi_ps(m2, m3);
        auto a1 = _mm_add_ps(u2, u3);
        auto f0 = _mm_movelh_ps(a0, a1);
        auto f1 = _mm_movehl_ps(a1, a0);
        auto f2 = _mm_add_ps(f0, f1);
        return float4(f2);
        #endif
    }

    template<> inline float4x4 operator*(const float4x4& m1, const float4x4& m2)
    {
        alignas(16) float4x4 result(0);
        for (auto i = 0u; i < 4u; ++i)
        {
            auto r = _mm_mul_ps(m1[0].data, _mm_shuffle_ps(m2[i].data, m2[i].data, _MM_SHUFFLE(0, 0, 0, 0)));
            r = _mm_add_ps(r, _mm_mul_ps(m1[1].data, _mm_shuffle_ps(m2[i].data, m2[i].data, _MM_SHUFFLE(1, 1, 1, 1))));
            r = _mm_add_ps(r, _mm_mul_ps(m1[2].data, _mm_shuffle_ps(m2[i].data, m2[i].data, _MM_SHUFFLE(2, 2, 2, 2))));
            r = _mm_add_ps(r, _mm_mul_ps(m1[3].data, _mm_shuffle_ps(m2[i].data, m2[i].data, _MM_SHUFFLE(3, 3, 3, 3))));
            result[i].data = r;
        }
        return result;
    }

    template<> inline float4 asfloat(const int4& v) { return float4(_mm_castsi128_ps(v.data)); }
    template<> inline float4 asfloat(const uint4& v) { return float4(_mm_castsi128_ps(v.data)); }
    template<> inline int4 asint(const float4& v) { return int4(_mm_castps_si128(v.data)); }
    template<> inline uint4 asuint(const float4& v) { return uint4(_mm_castps_si128(v.data)); }
    template<> inline float4 sqrt(const float4& v) { return float4(_mm_sqrt_ps(v.data)); }
    template<> inline float4 rsqrt(const float4& v) { return float4(_mm_rsqrt_ps(v.data)); }
    template<> inline float4 rcp(const float4& v) { return float4(_mm_rcp_ps(v.data)); }
    
    template<> inline float4 sign(const float4& v) 
    { 
        const auto zero = _mm_setzero_ps();
        const auto neg = _mm_and_ps(_mm_cmplt_ps(v.data, zero), _mm_set1_ps(-1.0f));
        const auto pos = _mm_and_ps(_mm_cmpgt_ps(v.data, zero), _mm_set1_ps(+1.0f));
        return float4(_mm_or_ps(neg, pos));
    }

    template<> inline float4 abs(const float4& v) { return float4(_mm_and_ps(v.data, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)))); }

    template<> inline int4 abs(const int4& v)
    {
        #if PK_MATH_SIMD_SSSE3
        return int4(_mm_sign_epi32(v.data, v.data));
        #else
        const auto mask = _mm_srai_epi32(v.data, 31);
        const auto xr = _mm_xor_si128(v.data, mask);
        return int4(_mm_sub_epi32(xr, mask));
        #endif
    }

    template<> inline float4 round(const float4& v) { return float4(_mm_cvtepi32_ps(_mm_cvtps_epi32(v.data))); }

    template<> inline float4 ceil(const float4& v) 
    {
        #if PK_MATH_SIMD_SSE4_1
        return float4(_mm_ceil_ps(v.data));
        #else
        const auto rnd = round(v).data;
        const auto cmp = _mm_cmpgt_ps(v.data, rnd);
        const auto xnd = _mm_and_ps(cmp, _mm_set1_ps(1.0f));
        return float4(_mm_add_ps(rnd, xnd));
        #endif
    }

    template<> inline float4 floor(const float4& v) 
    {
        #if PK_MATH_SIMD_SSE4_1
        return float4(_mm_floor_ps(v.data));
        #else
        const auto rnd = round(v).data;
        const auto cmp = _mm_cmplt_ps(v.data, rnd);
        const auto xnd = _mm_and_ps(cmp, _mm_set1_ps(1.0f));
        return float4(_mm_sub_ps(rnd, xnd));
        #endif
    }

    template<> inline float4 frac(const float4& v) { return float4(_mm_sub_ps(v.data, floor(v).data)); }
    
    template<> inline float4 mod(const float4& a, const float4& b)
    {
        const auto flr = floor(float4(_mm_div_ps(a.data, b.data))).data;
        return float4(_mm_sub_ps(a.data, _mm_mul_ps(b.data, flr)));
    }

    template<> inline float4 mod(const float4& a, float b)
    { 
        const auto vb = _mm_set1_ps(b);
        const auto flr = floor(float4(_mm_div_ps(a.data, vb))).data;
        return float4(_mm_sub_ps(a.data, _mm_mul_ps(vb, flr)));
    }

    template<> inline float4 fma(const float4& a, const float4& b, const float4& c) { return float4(_mm_add_ps(_mm_mul_ps(a.data, b.data), c.data)); }
    template<> inline float4 min(const float4& a, const float4& b) { return float4(_mm_min_ps(a.data, b.data)); } 
    template<> inline float4 min(const float4& a, float b) { return float4(_mm_min_ps(a.data, _mm_set1_ps(b))); }
    template<> inline float4 max(const float4& a, const float4& b) { return float4(_mm_max_ps(a.data, b.data)); }
    template<> inline float4 max(const float4& a, float b) { return float4(_mm_max_ps(a.data, _mm_set1_ps(b))); }

    template<> inline float4 lerp(const float4& a, const float4& b, const float4& i) { return float4(_mm_add_ps(a.data, _mm_mul_ps(i.data, _mm_sub_ps(b.data, a.data)))); }
    template<> inline float4 lerp(const float4& a, const float4& b, float i) { return float4(_mm_add_ps(a.data, _mm_mul_ps(_mm_set1_ps(i), _mm_sub_ps(b.data, a.data)))); }

    template<> inline float4 smoothstep(const float4& a, const float4& b, const float4& i)
    { 
        const auto div0 = _mm_sub_ps(_mm_sub_ps(i.data, a.data), _mm_sub_ps(b.data, a.data));
        const auto clp0 = _mm_max_ps(_mm_min_ps(div0, _mm_set1_ps(1.0f)), _mm_setzero_ps());
        const auto mul0 = _mm_mul_ps(_mm_set1_ps(2.0f), clp0);
        const auto sub2 = _mm_sub_ps(_mm_set1_ps(3.0f), mul0);
        const auto mul1 = _mm_mul_ps(clp0, clp0);
        return float4(_mm_mul_ps(mul1, sub2));
    }

    template<> inline float4 smoothstep(float a, float b, const float4& i)
    { 
        const auto div0 = _mm_sub_ps(_mm_sub_ps(i.data, _mm_set1_ps(a)), _mm_set1_ps(b - a));
        const auto clp0 = _mm_max_ps(_mm_min_ps(div0, _mm_set1_ps(1.0f)), _mm_setzero_ps());
        const auto mul0 = _mm_mul_ps(_mm_set1_ps(2.0f), clp0);
        const auto sub2 = _mm_sub_ps(_mm_set1_ps(3.0f), mul0);
        const auto mul1 = _mm_mul_ps(clp0, clp0);
        return float4(_mm_mul_ps(mul1, sub2));
    }

    template<> inline float dot(const float4& a, const float4& b) { return _mm_cvtss_f32(simd_dot_4(a.data, b.data)); }

    template<> inline float length(const float4& v) { return _mm_cvtss_f32(_mm_sqrt_ps(simd_dot_4(v.data, v.data))); }

    template<> inline float distance(const float4& a, const float4& b)
    { 
        auto v = _mm_sub_ps(a.data, b.data);
        return _mm_cvtss_f32(_mm_sqrt_ps(simd_dot_4(v, v)));
    }
    
    template<> inline float4 normalize(const float4& v) { return float4(_mm_mul_ps(v.data, _mm_rsqrt_ps(simd_dot_4(v.data, v.data)))); }

    template<> inline float3 cross(const float3& a, const float3& b)
    {
        const auto v1 = _mm_set_ps(0.0f, a.z, a.y, a.x);
        const auto v2 = _mm_set_ps(0.0f, b.z, b.y, b.x);
        const auto swp0 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 0, 2, 1));
        const auto swp1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(3, 1, 0, 2));
        const auto swp2 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3, 0, 2, 1));
        const auto swp3 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(3, 1, 0, 2));
        const auto mul0 = _mm_mul_ps(swp0, swp3);
        const auto mul1 = _mm_mul_ps(swp1, swp2);
        const auto sub0 = _mm_sub_ps(mul0, mul1);
        alignas(16) float4 result(sub0);
        return float3(result.x, result.y, result.z);
    }

    template<> inline float4 reflect(const float4& i, const float4& n) 
    { 
        const auto dot0 = simd_dot_4(n.data, i.data);
        const auto mul0 = _mm_mul_ps(i.data, dot0);
        const auto mul1 = _mm_mul_ps(mul0, _mm_set1_ps(2.0f));
        return float4(_mm_sub_ps(i.data, mul1));
    }

    template<> inline float4 refract(const float4& i, const float4& n, float eta)
    {
        const auto dot0 = simd_dot_4(n.data, i.data);
        const auto mul0 = _mm_set1_ps(eta * eta);
        const auto mul1 = _mm_mul_ps(dot0, dot0);
        const auto sub0 = _mm_sub_ps(_mm_set1_ps(1.0f), mul0);
        const auto sub1 = _mm_sub_ps(_mm_set1_ps(1.0f), mul1);
        const auto mul2 = _mm_mul_ps(sub0, sub1);

        if (_mm_movemask_ps(_mm_cmplt_ss(mul2, _mm_set1_ps(0.0f))) == 0)
        {
            return float4(0.0f);
        }

        const auto eta4 = _mm_set1_ps(eta);
        const auto sqt0 = _mm_sqrt_ps(mul2);
        const auto mad0 = _mm_add_ps(_mm_mul_ps(eta4, dot0), sqt0);
        const auto mul4 = _mm_mul_ps(mad0, n.data);
        const auto mul5 = _mm_mul_ps(eta4, i.data);
        return float4(_mm_sub_ps(mul5, mul4));
    }

    template<> inline float4x4 transpose(const float4x4& m)
    {   
        alignas(16) float4x4 result(0); 
        auto v0 = _mm_shuffle_ps(m[0].data, m[1].data, 0x44);
        auto v2 = _mm_shuffle_ps(m[0].data, m[1].data, 0xEE);
        auto v1 = _mm_shuffle_ps(m[2].data, m[3].data, 0x44);
        auto v3 = _mm_shuffle_ps(m[2].data, m[3].data, 0xEE);
        result[0].data = _mm_shuffle_ps(v0, v1, 0x88);
        result[1].data = _mm_shuffle_ps(v0, v1, 0xDD);
        result[2].data = _mm_shuffle_ps(v2, v3, 0x88);
        result[3].data = _mm_shuffle_ps(v2, v3, 0xDD);
        return result; 
    }

    template<> inline float3x4 transpose3x4(const float4x4& m)
    {
        alignas(16) float3x4 result(0);
        auto v0 = _mm_shuffle_ps(m[0].data, m[1].data, 0x44);
        auto v2 = _mm_shuffle_ps(m[0].data, m[1].data, 0xEE);
        auto v1 = _mm_shuffle_ps(m[2].data, m[3].data, 0x44);
        auto v3 = _mm_shuffle_ps(m[2].data, m[3].data, 0xEE);
        result[0].data = _mm_shuffle_ps(v0, v1, 0x88);
        result[1].data = _mm_shuffle_ps(v0, v1, 0xDD);
        result[2].data = _mm_shuffle_ps(v2, v3, 0x88);
        return result;
    }

    template<> inline float determinant(const float4x4& m)
    {
        const auto m0 = m[0].data;
        const auto m1 = m[1].data;
        const auto m2 = m[2].data;
        const auto m3 = m[3].data;
        auto n0 = _mm_mul_ps(_mm_shuffle_ps(m2, m2, _MM_SHUFFLE(0, 1, 1, 2)), _mm_shuffle_ps(m3, m3, _MM_SHUFFLE(3, 2, 3, 3)));
        auto n1 = _mm_mul_ps(_mm_shuffle_ps(m2, m2, _MM_SHUFFLE(3, 2, 3, 3)), _mm_shuffle_ps(m3, m3, _MM_SHUFFLE(0, 1, 1, 2)));
        auto n2 = _mm_mul_ps(_mm_shuffle_ps(m2, m2, _MM_SHUFFLE(0, 0, 1, 2)), _mm_shuffle_ps(m3, m3, _MM_SHUFFLE(1, 2, 0, 0)));
        auto s0 = _mm_sub_ps(n0, n1);
        auto s1 = _mm_sub_ps(_mm_movehl_ps(n2, n2), n2);
        auto s2 = _mm_shuffle_ps(s0, s1, _MM_SHUFFLE(0, 0, 3, 1));
        auto s3 = _mm_shuffle_ps(s0, s1, _MM_SHUFFLE(1, 0, 2, 2));
        auto n3 = _mm_mul_ps(_mm_shuffle_ps(m1, m1, _MM_SHUFFLE(0, 0, 0, 1)), _mm_shuffle_ps(s0, s0, _MM_SHUFFLE(2, 1, 0, 0)));
        auto n4 = _mm_mul_ps(_mm_shuffle_ps(m1, m1, _MM_SHUFFLE(1, 1, 2, 2)), _mm_shuffle_ps(s2, s2, _MM_SHUFFLE(3, 1, 1, 0)));
        auto n5 = _mm_mul_ps(_mm_shuffle_ps(m1, m1, _MM_SHUFFLE(2, 3, 3, 3)), _mm_shuffle_ps(s3, s3, _MM_SHUFFLE(3, 3, 2, 0)));
        auto dc = _mm_mul_ps(_mm_add_ps(_mm_sub_ps(n3, n4), n5), _mm_setr_ps( 1.0f,-1.0f, 1.0f,-1.0f));
        return _mm_cvtss_f32(simd_dot_4(m0, dc));
    }

    template<> inline float4x4 inverse(const float4x4& m)
    {
        const auto m0 = m[0].data;
        const auto m1 = m[1].data;
        const auto m2 = m[2].data;
        const auto m3 = m[3].data;
        simd_f32vec4 t0, t1;

        t0 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(3, 3, 3, 3));
        t0 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 0));
        t0 = _mm_mul_ps(t0, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(2, 2, 2, 2)));
        t1 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(2, 2, 2, 2));
        t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 0, 0, 0));
        t1 = _mm_mul_ps(t1, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(3, 3, 3, 3)));
        auto f0 = _mm_sub_ps(t0, t1);

        t0 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(3, 3, 3, 3));
        t0 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 0));
        t0 = _mm_mul_ps(t0, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(1, 1, 1, 1)));
        t1 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(1, 1, 1, 1));
        t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 0, 0, 0));
        t1 = _mm_mul_ps(t1, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(3, 3, 3, 3)));
        auto f1 = _mm_sub_ps(t0, t1);

        t0 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(2, 2, 2, 2));
        t0 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 0));
        t0 = _mm_mul_ps(t0, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(1, 1, 1, 1)));
        t1 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(1, 1, 1, 1));
        t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 0, 0, 0));
        t1 = _mm_mul_ps(t1, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(2, 2, 2, 2)));
        auto f2 = _mm_sub_ps(t0, t1);

        t0 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(3, 3, 3, 3));
        t0 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 0));
        t0 = _mm_mul_ps(t0, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(0, 0, 0, 0)));
        t1 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(0, 0, 0, 0));
        t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 0, 0, 0));
        t1 = _mm_mul_ps(t1, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(3, 3, 3, 3)));
        auto f3 = _mm_sub_ps(t0, t1);

        t0 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(2, 2, 2, 2));
        t0 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 0));
        t0 = _mm_mul_ps(t0, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(0, 0, 0, 0)));
        t1 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(0, 0, 0, 0));
        t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 0, 0, 0));
        t1 = _mm_mul_ps(t1, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(2, 2, 2, 2)));
        auto f4 = _mm_sub_ps(t0, t1);

        t0 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(1, 1, 1, 1));
        t0 = _mm_shuffle_ps(t0, t0, _MM_SHUFFLE(2, 0, 0, 0));
        t0 = _mm_mul_ps(t0, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(0, 0, 0, 0)));
        t1 = _mm_shuffle_ps(m3, m2, _MM_SHUFFLE(0, 0, 0, 0));
        t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(2, 0, 0, 0));
        t1 = _mm_mul_ps(t1, _mm_shuffle_ps(m2, m1, _MM_SHUFFLE(1, 1, 1, 1)));
        auto f5 = _mm_sub_ps(t0, t1);

        auto v0 = _mm_shuffle_ps(m1, m0, _MM_SHUFFLE(0, 0, 0, 0));
        v0 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(2, 2, 2, 0));
        auto v1 = _mm_shuffle_ps(m1, m0, _MM_SHUFFLE(1, 1, 1, 1));
        v1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2, 2, 2, 0));
        auto v2 = _mm_shuffle_ps(m1, m0, _MM_SHUFFLE(2, 2, 2, 2));
        v2 = _mm_shuffle_ps(v2, v2, _MM_SHUFFLE(2, 2, 2, 0));
        auto v3 = _mm_shuffle_ps(m1, m0, _MM_SHUFFLE(3, 3, 3, 3));
        v3 = _mm_shuffle_ps(v3, v3, _MM_SHUFFLE(2, 2, 2, 0));

        auto s0 = _mm_set_ps(-1.0f, 1.0f, -1.0f, 1.0f);
        auto i0 = _mm_sub_ps(_mm_mul_ps(v1, f0), _mm_mul_ps(v2, f1));
        i0 = _mm_mul_ps(_mm_add_ps(i0, _mm_mul_ps(v3, f2)), s0);
        
        auto i2 = _mm_sub_ps(_mm_mul_ps(v0, f1), _mm_mul_ps(v1, f3));
        i2 = _mm_mul_ps(_mm_add_ps(i2, _mm_mul_ps(v3, f5)), s0);
        
        auto s1 = _mm_set_ps(1.0f, -1.0f, 1.0f, -1.0f);
        auto i1 = _mm_sub_ps(_mm_mul_ps(v0, f0), _mm_mul_ps(v2, f3));
        i1 = _mm_mul_ps(_mm_add_ps(i1, _mm_mul_ps(v3, f4)), s1);
        
        auto i3 = _mm_sub_ps(_mm_mul_ps(v0, f2), _mm_mul_ps(v1, f4));
        i3 = _mm_mul_ps(_mm_add_ps(i3, _mm_mul_ps(v2, f5)), s1);

        auto r0 = _mm_shuffle_ps(i0, i1, _MM_SHUFFLE(0, 0, 0, 0));
        auto r1 = _mm_shuffle_ps(i2, i3, _MM_SHUFFLE(0, 0, 0, 0));
        auto r2 = _mm_shuffle_ps(r0, r1, _MM_SHUFFLE(2, 0, 2, 0));
        auto id = _mm_div_ps(_mm_set1_ps(1.0f), simd_dot_4(m0, r2));

        alignas(16) float4x4 result(0);
        result[0].data = _mm_mul_ps(i0, id);
        result[1].data = _mm_mul_ps(i1, id);
        result[2].data = _mm_mul_ps(i2, id);
        result[3].data = _mm_mul_ps(i3, id);
        return result;
    }
}
#endif
