#pragma once
#include "Scalar.h"

namespace PK::math
{
    /*
        ================================================================================
        OpenGL Mathematics (GLM)
        --------------------------------------------------------------------------------
        GLM is licensed under The Happy Bunny License or MIT License
        
        ================================================================================
        The Happy Bunny License (Modified MIT License)
        --------------------------------------------------------------------------------
        Copyright (c) 2005 - G-Truc Creation
        
        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:
        
        The above copyright notice and this permission notice shall be included in
        all copies or substantial portions of the Software.
        
        Restrictions:
         By making use of the Software for military purposes, you choose to make a
         Bunny unhappy.
        
        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
        THE SOFTWARE.
    */

    // stripped down glm vector & matrix types
    // removed most code to reduce binary size

    template<typename T, int N> struct storage { constexpr static const bool is_simd = false; typedef struct type { T data[N]; } type; };

    #if defined(PK_MATH_SIMD_SSE2)
    template<> struct storage<float,4> { constexpr static const bool is_simd = true; typedef simd_f32vec4 type; };
    template<> struct storage<double,2> { constexpr static const bool is_simd = true; typedef simd_f64vec2 type; };
    template<> struct storage<int32_t,4> { constexpr static const bool is_simd = true; typedef simd_i32vec4 type; };
    template<> struct storage<uint32_t,4> { constexpr static const bool is_simd = true; typedef simd_u32vec4 type; };
    template<> struct storage<int64_t,2> { constexpr static const bool is_simd = true; typedef simd_i64vec2 type; };
    template<> struct storage<uint64_t,2> { constexpr static const bool is_simd = true; typedef simd_u64vec2 type; };
    #endif

    #if defined(PK_MATH_SIMD_NEON)
    template<> struct storage<float,4> { constexpr static const bool is_simd = true; typedef simd_f32vec4 type; };
    template<> struct storage<int32_t,4> { constexpr static const bool is_simd = true; typedef simd_i32vec4 type; };
    template<> struct storage<uint32_t,4> { constexpr static const bool is_simd = true; typedef glm_u32vec4 type; };
    #endif

    template<typename T, int N>
    struct swizzle_0
    {
        inline T& elem(size_t i) { return (reinterpret_cast<T*>(buffer))[i]; }
        inline const T& elem(size_t i) const { return (reinterpret_cast<const T*>(buffer))[i]; }
        char buffer[1];
    };

    template<typename T,int N, int E0, int E1, int E2, int E3> struct swizzle_1 : protected swizzle_0<T, N> {};
    template<typename T,int E0,int E1> struct swizzle_1<T,2,E0,E1,-1,-2> : protected swizzle_0<T,2> { inline vector<T,2> operator()() const { return vector<T,2>(this->elem(E0), this->elem(E1));}};
    template<typename T,int E0,int E1,int E2> struct swizzle_1<T,3,E0,E1,E2,-1> : protected swizzle_0<T,3> { inline vector<T,3> operator()()  const { return vector<T,3>(this->elem(E0), this->elem(E1), this->elem(E2));}};
    template<typename T,int E0,int E1,int E2,int E3> struct swizzle_1<T,4,E0,E1,E2,E3> : protected swizzle_0<T,4> { inline vector<T,4> operator()()  const { return vector<T,4>(this->elem(E0), this->elem(E1), this->elem(E2), this->elem(E3));}};

    template<typename T,int N,int E0,int E1,int E2,int E3, int DUPLICATE_ELEMENTS>
    struct swizzle_2 : public swizzle_1<T, N, E0, E1, E2, E3>
    {
        struct op_equal { inline void operator() (T& e, T& t) const { e = t; } };
        struct op_minus { inline void operator() (T& e, T& t) const { e -= t; } };
        struct op_plus { inline void operator() (T& e, T& t) const { e += t; } };
        struct op_mul { inline void operator() (T& e, T& t) const { e *= t; } };
        struct op_div { inline void operator() (T& e, T& t) const { e /= t; } };
        inline swizzle_2& operator= (T v) { for (int i = 0; i < N; ++i) (*this)[i] = v; return *this; }
        inline swizzle_2& operator= (const vector<T,N>& that) { apply_op(that, op_equal()); return *this; }
        inline void operator -= (const vector<T,N>& that) { apply_op(that, op_minus()); }
        inline void operator += (const vector<T,N>& that) { apply_op(that, op_plus()); }
        inline void operator *= (const vector<T,N>& that) { apply_op(that, op_mul()); }
        inline void operator /= (const vector<T,N>& that) { apply_op(that, op_div()); }
        inline void operator -= (T v) { for (int i = 0; i < N; ++i) (*this)[i] -= v; }
        inline void operator += (T v) { for (int i = 0; i < N; ++i) (*this)[i] += v; }
        inline void operator *= (T v) { for (int i = 0; i < N; ++i) (*this)[i] *= v; }
        inline void operator /= (T v) { for (int i = 0; i < N; ++i) (*this)[i] /= v; }
        inline T& operator[](size_t i) { constexpr const int offset_dst[4] = { E0, E1, E2, E3 }; return this->elem(offset_dst[i]); }
        inline T operator[](size_t i) const { constexpr const int offset_dst[4] = { E0, E1, E2, E3 }; return this->elem(offset_dst[i]); }

    protected:
        template<typename U>
        inline void apply_op(const vector<T,N>& that, const U& op)
        {
            // copy data in case l/r are the same vector.
            T t[N];
            for (int i = 0; i < N; ++i) t[i] = that[i];
            for (int i = 0; i < N; ++i) op((*this)[i], t[i]);
        }
    };

    template<typename T, int N, int E0, int E1, int E2, int E3>
    struct swizzle_2<T, N, E0, E1, E2, E3, 1> : public swizzle_1<T, N, E0, E1, E2, E3>
    {
        struct Stub {};
        inline swizzle_2& operator= (Stub const&) { return *this; }
        inline T operator[] (size_t i) const { constexpr const int offset_dst[4] = { E0, E1, E2, E3 }; return this->elem(offset_dst[i]); }
    };

    template<typename T, int N, int E0, int E1, int E2, int E3>
    struct swizzle : public swizzle_2<T, N, E0, E1, E2, E3, (E0 == E1 || E0 == E2 || E0 == E3 || E1 == E2 || E1 == E3 || E2 == E3)>
    {
        typedef swizzle_2<T, N, E0, E1, E2, E3, (E0 == E1 || E0 == E2 || E0 == E3 || E1 == E2 || E1 == E3 || E2 == E3)> base_type;
        using base_type::operator=;
        inline operator vector<T,N>() const { return (*this)(); }
    };

    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator+(const swizzle<T,N,E0,E1,E2,E3>& a, const T& b) { return a() + b; }
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator+(const T& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a + b(); }
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator-(const swizzle<T,N,E0,E1,E2,E3>& a, const T& b) { return a() - b;}
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator-(const T& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a - b(); }
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator*(const swizzle<T,N,E0,E1,E2,E3>& a, const T& b) { return a() * b; }
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator*(const T& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a * b(); }
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator/(const swizzle<T,N,E0,E1,E2,E3>& a, const T& b) { return a() / b; }
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator/(const T& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a / b(); }

    template<typename T,int N,int E0,int E1,int E2,int E3,int F0,int F1,int F2,int F3> inline vector<T,N> operator+(const swizzle<T,N,E0,E1,E2,E3>& a, const swizzle<T,N,F0,F1,F2,F3>& b) { return a() + b(); }  
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator+(const swizzle<T,N,E0,E1,E2,E3>& a, const vector<T,N>& b) { return a() + b; } 
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator+(const vector<T,N>& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a + b(); }

    template<typename T,int N,int E0,int E1,int E2,int E3,int F0,int F1,int F2,int F3> inline vector<T,N> operator-(const swizzle<T,N,E0,E1,E2,E3>& a, const swizzle<T,N,F0,F1,F2,F3>& b) { return a() - b(); }  
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator-(const swizzle<T,N,E0,E1,E2,E3>& a, const vector<T,N>& b) { return a() - b; } 
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator-(const vector<T,N>& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a - b(); }

    template<typename T,int N,int E0,int E1,int E2,int E3,int F0,int F1,int F2,int F3> inline vector<T,N> operator*(const swizzle<T,N,E0,E1,E2,E3>& a, const swizzle<T,N,F0,F1,F2,F3>& b) { return a() * b(); }  
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator*(const swizzle<T,N,E0,E1,E2,E3>& a, const vector<T,N>& b) { return a() * b; } 
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator*(const vector<T,N>& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a * b(); }

    template<typename T,int N,int E0,int E1,int E2,int E3,int F0,int F1,int F2,int F3> inline vector<T,N> operator/(const swizzle<T,N,E0,E1,E2,E3>& a, const swizzle<T,N,F0,F1,F2,F3>& b) { return a() / b(); }  
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator/(const swizzle<T,N,E0,E1,E2,E3>& a, const vector<T,N>& b) { return a() / b; } 
    template<typename T,int N,int E0,int E1,int E2,int E3> inline vector<T,N> operator/(const vector<T,N>& a, const swizzle<T,N,E0,E1,E2,E3>& b) { return a / b(); }

#define PK_SWIZZLE_MEMBERS_22(T,E0,E1) \
    struct { swizzle<T,2, 0,0,-1,-2> E0 ## E0; }; \
    struct { swizzle<T,2, 0,1,-1,-2> E0 ## E1; }; \
    struct { swizzle<T,2, 1,0,-1,-2> E1 ## E0; }; \
    struct { swizzle<T,2, 1,1,-1,-2> E1 ## E1; };

#define PK_SWIZZLE_MEMBERS_23(T,E0,E1) \
    struct { swizzle<T,3, 0,0,0,-1> E0 ## E0 ## E0; }; \
    struct { swizzle<T,3, 0,0,1,-1> E0 ## E0 ## E1; }; \
    struct { swizzle<T,3, 0,1,0,-1> E0 ## E1 ## E0; }; \
    struct { swizzle<T,3, 0,1,1,-1> E0 ## E1 ## E1; }; \
    struct { swizzle<T,3, 1,0,0,-1> E1 ## E0 ## E0; }; \
    struct { swizzle<T,3, 1,0,1,-1> E1 ## E0 ## E1; }; \
    struct { swizzle<T,3, 1,1,0,-1> E1 ## E1 ## E0; }; \
    struct { swizzle<T,3, 1,1,1,-1> E1 ## E1 ## E1; };

#define PK_SWIZZLE_MEMBERS_24(T,E0,E1) \
    struct { swizzle<T,4, 0,0,0,0> E0 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,0,0,1> E0 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,0,1,0> E0 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,0,1,1> E0 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,1,0,0> E0 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,1,0,1> E0 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,1,1,0> E0 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,1,1,1> E0 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,0,0,0> E1 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,0,0,1> E1 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,0,1,0> E1 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,0,1,1> E1 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,1,0,0> E1 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,1,0,1> E1 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,1,1,0> E1 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,1,1,1> E1 ## E1 ## E1 ## E1; };

#define PK_SWIZZLE_MEMBERS_32(T,E0,E1,E2) \
    struct { swizzle<T,2, 0,0,-1,-2> E0 ## E0; }; \
    struct { swizzle<T,2, 0,1,-1,-2> E0 ## E1; }; \
    struct { swizzle<T,2, 0,2,-1,-2> E0 ## E2; }; \
    struct { swizzle<T,2, 1,0,-1,-2> E1 ## E0; }; \
    struct { swizzle<T,2, 1,1,-1,-2> E1 ## E1; }; \
    struct { swizzle<T,2, 1,2,-1,-2> E1 ## E2; }; \
    struct { swizzle<T,2, 2,0,-1,-2> E2 ## E0; }; \
    struct { swizzle<T,2, 2,1,-1,-2> E2 ## E1; }; \
    struct { swizzle<T,2, 2,2,-1,-2> E2 ## E2; };

#define PK_SWIZZLE_MEMBERS_33(T,E0,E1,E2) \
    struct { swizzle<T,3, 0,0,0,-1> E0 ## E0 ## E0; }; \
    struct { swizzle<T,3, 0,0,1,-1> E0 ## E0 ## E1; }; \
    struct { swizzle<T,3, 0,0,2,-1> E0 ## E0 ## E2; }; \
    struct { swizzle<T,3, 0,1,0,-1> E0 ## E1 ## E0; }; \
    struct { swizzle<T,3, 0,1,1,-1> E0 ## E1 ## E1; }; \
    struct { swizzle<T,3, 0,1,2,-1> E0 ## E1 ## E2; }; \
    struct { swizzle<T,3, 0,2,0,-1> E0 ## E2 ## E0; }; \
    struct { swizzle<T,3, 0,2,1,-1> E0 ## E2 ## E1; }; \
    struct { swizzle<T,3, 0,2,2,-1> E0 ## E2 ## E2; }; \
    struct { swizzle<T,3, 1,0,0,-1> E1 ## E0 ## E0; }; \
    struct { swizzle<T,3, 1,0,1,-1> E1 ## E0 ## E1; }; \
    struct { swizzle<T,3, 1,0,2,-1> E1 ## E0 ## E2; }; \
    struct { swizzle<T,3, 1,1,0,-1> E1 ## E1 ## E0; }; \
    struct { swizzle<T,3, 1,1,1,-1> E1 ## E1 ## E1; }; \
    struct { swizzle<T,3, 1,1,2,-1> E1 ## E1 ## E2; }; \
    struct { swizzle<T,3, 1,2,0,-1> E1 ## E2 ## E0; }; \
    struct { swizzle<T,3, 1,2,1,-1> E1 ## E2 ## E1; }; \
    struct { swizzle<T,3, 1,2,2,-1> E1 ## E2 ## E2; }; \
    struct { swizzle<T,3, 2,0,0,-1> E2 ## E0 ## E0; }; \
    struct { swizzle<T,3, 2,0,1,-1> E2 ## E0 ## E1; }; \
    struct { swizzle<T,3, 2,0,2,-1> E2 ## E0 ## E2; }; \
    struct { swizzle<T,3, 2,1,0,-1> E2 ## E1 ## E0; }; \
    struct { swizzle<T,3, 2,1,1,-1> E2 ## E1 ## E1; }; \
    struct { swizzle<T,3, 2,1,2,-1> E2 ## E1 ## E2; }; \
    struct { swizzle<T,3, 2,2,0,-1> E2 ## E2 ## E0; }; \
    struct { swizzle<T,3, 2,2,1,-1> E2 ## E2 ## E1; }; \
    struct { swizzle<T,3, 2,2,2,-1> E2 ## E2 ## E2; };

#define PK_SWIZZLE_MEMBERS_34(T,E0,E1,E2) \
    struct { swizzle<T,4, 0,0,0,0> E0 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,0,0,1> E0 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,0,0,2> E0 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,0,1,0> E0 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,0,1,1> E0 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,0,1,2> E0 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,0,2,0> E0 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,0,2,1> E0 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,0,2,2> E0 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 0,1,0,0> E0 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,1,0,1> E0 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,1,0,2> E0 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,1,1,0> E0 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,1,1,1> E0 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,1,1,2> E0 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,1,2,0> E0 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,1,2,1> E0 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,1,2,2> E0 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 0,2,0,0> E0 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,2,0,1> E0 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,2,0,2> E0 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,2,1,0> E0 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,2,1,1> E0 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,2,1,2> E0 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,2,2,0> E0 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,2,2,1> E0 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,2,2,2> E0 ## E2 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,0,0,0> E1 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,0,0,1> E1 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,0,0,2> E1 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,0,1,0> E1 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,0,1,1> E1 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,0,1,2> E1 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,0,2,0> E1 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,0,2,1> E1 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,0,2,2> E1 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,1,0,0> E1 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,1,0,1> E1 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,1,0,2> E1 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,1,1,0> E1 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,1,1,1> E1 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,1,1,2> E1 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,1,2,0> E1 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,1,2,1> E1 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,1,2,2> E1 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,2,0,0> E1 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,2,0,1> E1 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,2,0,2> E1 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,2,1,0> E1 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,2,1,1> E1 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,2,1,2> E1 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,2,2,0> E1 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,2,2,1> E1 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,2,2,2> E1 ## E2 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,0,0,0> E2 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,0,0,1> E2 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,0,0,2> E2 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,0,1,0> E2 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,0,1,1> E2 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,0,1,2> E2 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,0,2,0> E2 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,0,2,1> E2 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,0,2,2> E2 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,1,0,0> E2 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,1,0,1> E2 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,1,0,2> E2 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,1,1,0> E2 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,1,1,1> E2 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,1,1,2> E2 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,1,2,0> E2 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,1,2,1> E2 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,1,2,2> E2 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,2,0,0> E2 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,2,0,1> E2 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,2,0,2> E2 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,2,1,0> E2 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,2,1,1> E2 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,2,1,2> E2 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,2,2,0> E2 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,2,2,1> E2 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,2,2,2> E2 ## E2 ## E2 ## E2; };

#define PK_SWIZZLE_MEMBERS_42(T,E0,E1,E2,E3) \
    struct { swizzle<T,2, 0,0,-1,-2> E0 ## E0; }; \
    struct { swizzle<T,2, 0,1,-1,-2> E0 ## E1; }; \
    struct { swizzle<T,2, 0,2,-1,-2> E0 ## E2; }; \
    struct { swizzle<T,2, 0,3,-1,-2> E0 ## E3; }; \
    struct { swizzle<T,2, 1,0,-1,-2> E1 ## E0; }; \
    struct { swizzle<T,2, 1,1,-1,-2> E1 ## E1; }; \
    struct { swizzle<T,2, 1,2,-1,-2> E1 ## E2; }; \
    struct { swizzle<T,2, 1,3,-1,-2> E1 ## E3; }; \
    struct { swizzle<T,2, 2,0,-1,-2> E2 ## E0; }; \
    struct { swizzle<T,2, 2,1,-1,-2> E2 ## E1; }; \
    struct { swizzle<T,2, 2,2,-1,-2> E2 ## E2; }; \
    struct { swizzle<T,2, 2,3,-1,-2> E2 ## E3; }; \
    struct { swizzle<T,2, 3,0,-1,-2> E3 ## E0; }; \
    struct { swizzle<T,2, 3,1,-1,-2> E3 ## E1; }; \
    struct { swizzle<T,2, 3,2,-1,-2> E3 ## E2; }; \
    struct { swizzle<T,2, 3,3,-1,-2> E3 ## E3; };

#define PK_SWIZZLE_MEMBERS_43(T,E0,E1,E2,E3) \
    struct { swizzle<T,3, 0,0,0,-1> E0 ## E0 ## E0; }; \
    struct { swizzle<T,3, 0,0,1,-1> E0 ## E0 ## E1; }; \
    struct { swizzle<T,3, 0,0,2,-1> E0 ## E0 ## E2; }; \
    struct { swizzle<T,3, 0,0,3,-1> E0 ## E0 ## E3; }; \
    struct { swizzle<T,3, 0,1,0,-1> E0 ## E1 ## E0; }; \
    struct { swizzle<T,3, 0,1,1,-1> E0 ## E1 ## E1; }; \
    struct { swizzle<T,3, 0,1,2,-1> E0 ## E1 ## E2; }; \
    struct { swizzle<T,3, 0,1,3,-1> E0 ## E1 ## E3; }; \
    struct { swizzle<T,3, 0,2,0,-1> E0 ## E2 ## E0; }; \
    struct { swizzle<T,3, 0,2,1,-1> E0 ## E2 ## E1; }; \
    struct { swizzle<T,3, 0,2,2,-1> E0 ## E2 ## E2; }; \
    struct { swizzle<T,3, 0,2,3,-1> E0 ## E2 ## E3; }; \
    struct { swizzle<T,3, 0,3,0,-1> E0 ## E3 ## E0; }; \
    struct { swizzle<T,3, 0,3,1,-1> E0 ## E3 ## E1; }; \
    struct { swizzle<T,3, 0,3,2,-1> E0 ## E3 ## E2; }; \
    struct { swizzle<T,3, 0,3,3,-1> E0 ## E3 ## E3; }; \
    struct { swizzle<T,3, 1,0,0,-1> E1 ## E0 ## E0; }; \
    struct { swizzle<T,3, 1,0,1,-1> E1 ## E0 ## E1; }; \
    struct { swizzle<T,3, 1,0,2,-1> E1 ## E0 ## E2; }; \
    struct { swizzle<T,3, 1,0,3,-1> E1 ## E0 ## E3; }; \
    struct { swizzle<T,3, 1,1,0,-1> E1 ## E1 ## E0; }; \
    struct { swizzle<T,3, 1,1,1,-1> E1 ## E1 ## E1; }; \
    struct { swizzle<T,3, 1,1,2,-1> E1 ## E1 ## E2; }; \
    struct { swizzle<T,3, 1,1,3,-1> E1 ## E1 ## E3; }; \
    struct { swizzle<T,3, 1,2,0,-1> E1 ## E2 ## E0; }; \
    struct { swizzle<T,3, 1,2,1,-1> E1 ## E2 ## E1; }; \
    struct { swizzle<T,3, 1,2,2,-1> E1 ## E2 ## E2; }; \
    struct { swizzle<T,3, 1,2,3,-1> E1 ## E2 ## E3; }; \
    struct { swizzle<T,3, 1,3,0,-1> E1 ## E3 ## E0; }; \
    struct { swizzle<T,3, 1,3,1,-1> E1 ## E3 ## E1; }; \
    struct { swizzle<T,3, 1,3,2,-1> E1 ## E3 ## E2; }; \
    struct { swizzle<T,3, 1,3,3,-1> E1 ## E3 ## E3; }; \
    struct { swizzle<T,3, 2,0,0,-1> E2 ## E0 ## E0; }; \
    struct { swizzle<T,3, 2,0,1,-1> E2 ## E0 ## E1; }; \
    struct { swizzle<T,3, 2,0,2,-1> E2 ## E0 ## E2; }; \
    struct { swizzle<T,3, 2,0,3,-1> E2 ## E0 ## E3; }; \
    struct { swizzle<T,3, 2,1,0,-1> E2 ## E1 ## E0; }; \
    struct { swizzle<T,3, 2,1,1,-1> E2 ## E1 ## E1; }; \
    struct { swizzle<T,3, 2,1,2,-1> E2 ## E1 ## E2; }; \
    struct { swizzle<T,3, 2,1,3,-1> E2 ## E1 ## E3; }; \
    struct { swizzle<T,3, 2,2,0,-1> E2 ## E2 ## E0; }; \
    struct { swizzle<T,3, 2,2,1,-1> E2 ## E2 ## E1; }; \
    struct { swizzle<T,3, 2,2,2,-1> E2 ## E2 ## E2; }; \
    struct { swizzle<T,3, 2,2,3,-1> E2 ## E2 ## E3; }; \
    struct { swizzle<T,3, 2,3,0,-1> E2 ## E3 ## E0; }; \
    struct { swizzle<T,3, 2,3,1,-1> E2 ## E3 ## E1; }; \
    struct { swizzle<T,3, 2,3,2,-1> E2 ## E3 ## E2; }; \
    struct { swizzle<T,3, 2,3,3,-1> E2 ## E3 ## E3; }; \
    struct { swizzle<T,3, 3,0,0,-1> E3 ## E0 ## E0; }; \
    struct { swizzle<T,3, 3,0,1,-1> E3 ## E0 ## E1; }; \
    struct { swizzle<T,3, 3,0,2,-1> E3 ## E0 ## E2; }; \
    struct { swizzle<T,3, 3,0,3,-1> E3 ## E0 ## E3; }; \
    struct { swizzle<T,3, 3,1,0,-1> E3 ## E1 ## E0; }; \
    struct { swizzle<T,3, 3,1,1,-1> E3 ## E1 ## E1; }; \
    struct { swizzle<T,3, 3,1,2,-1> E3 ## E1 ## E2; }; \
    struct { swizzle<T,3, 3,1,3,-1> E3 ## E1 ## E3; }; \
    struct { swizzle<T,3, 3,2,0,-1> E3 ## E2 ## E0; }; \
    struct { swizzle<T,3, 3,2,1,-1> E3 ## E2 ## E1; }; \
    struct { swizzle<T,3, 3,2,2,-1> E3 ## E2 ## E2; }; \
    struct { swizzle<T,3, 3,2,3,-1> E3 ## E2 ## E3; }; \
    struct { swizzle<T,3, 3,3,0,-1> E3 ## E3 ## E0; }; \
    struct { swizzle<T,3, 3,3,1,-1> E3 ## E3 ## E1; }; \
    struct { swizzle<T,3, 3,3,2,-1> E3 ## E3 ## E2; }; \
    struct { swizzle<T,3, 3,3,3,-1> E3 ## E3 ## E3; };

#define PK_SWIZZLE_MEMBERS_44(T,E0,E1,E2,E3) \
    struct { swizzle<T,4, 0,0,0,0> E0 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,0,0,1> E0 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,0,0,2> E0 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,0,0,3> E0 ## E0 ## E0 ## E3; }; \
    struct { swizzle<T,4, 0,0,1,0> E0 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,0,1,1> E0 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,0,1,2> E0 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,0,1,3> E0 ## E0 ## E1 ## E3; }; \
    struct { swizzle<T,4, 0,0,2,0> E0 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,0,2,1> E0 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,0,2,2> E0 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 0,0,2,3> E0 ## E0 ## E2 ## E3; }; \
    struct { swizzle<T,4, 0,0,3,0> E0 ## E0 ## E3 ## E0; }; \
    struct { swizzle<T,4, 0,0,3,1> E0 ## E0 ## E3 ## E1; }; \
    struct { swizzle<T,4, 0,0,3,2> E0 ## E0 ## E3 ## E2; }; \
    struct { swizzle<T,4, 0,0,3,3> E0 ## E0 ## E3 ## E3; }; \
    struct { swizzle<T,4, 0,1,0,0> E0 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,1,0,1> E0 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,1,0,2> E0 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,1,0,3> E0 ## E1 ## E0 ## E3; }; \
    struct { swizzle<T,4, 0,1,1,0> E0 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,1,1,1> E0 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,1,1,2> E0 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,1,1,3> E0 ## E1 ## E1 ## E3; }; \
    struct { swizzle<T,4, 0,1,2,0> E0 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,1,2,1> E0 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,1,2,2> E0 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 0,1,2,3> E0 ## E1 ## E2 ## E3; }; \
    struct { swizzle<T,4, 0,1,3,0> E0 ## E1 ## E3 ## E0; }; \
    struct { swizzle<T,4, 0,1,3,1> E0 ## E1 ## E3 ## E1; }; \
    struct { swizzle<T,4, 0,1,3,2> E0 ## E1 ## E3 ## E2; }; \
    struct { swizzle<T,4, 0,1,3,3> E0 ## E1 ## E3 ## E3; }; \
    struct { swizzle<T,4, 0,2,0,0> E0 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,2,0,1> E0 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,2,0,2> E0 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,2,0,3> E0 ## E2 ## E0 ## E3; }; \
    struct { swizzle<T,4, 0,2,1,0> E0 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,2,1,1> E0 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,2,1,2> E0 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,2,1,3> E0 ## E2 ## E1 ## E3; }; \
    struct { swizzle<T,4, 0,2,2,0> E0 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,2,2,1> E0 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,2,2,2> E0 ## E2 ## E2 ## E2; }; \
    struct { swizzle<T,4, 0,2,2,3> E0 ## E2 ## E2 ## E3; }; \
    struct { swizzle<T,4, 0,2,3,0> E0 ## E2 ## E3 ## E0; }; \
    struct { swizzle<T,4, 0,2,3,1> E0 ## E2 ## E3 ## E1; }; \
    struct { swizzle<T,4, 0,2,3,2> E0 ## E2 ## E3 ## E2; }; \
    struct { swizzle<T,4, 0,2,3,3> E0 ## E2 ## E3 ## E3; }; \
    struct { swizzle<T,4, 0,3,0,0> E0 ## E3 ## E0 ## E0; }; \
    struct { swizzle<T,4, 0,3,0,1> E0 ## E3 ## E0 ## E1; }; \
    struct { swizzle<T,4, 0,3,0,2> E0 ## E3 ## E0 ## E2; }; \
    struct { swizzle<T,4, 0,3,0,3> E0 ## E3 ## E0 ## E3; }; \
    struct { swizzle<T,4, 0,3,1,0> E0 ## E3 ## E1 ## E0; }; \
    struct { swizzle<T,4, 0,3,1,1> E0 ## E3 ## E1 ## E1; }; \
    struct { swizzle<T,4, 0,3,1,2> E0 ## E3 ## E1 ## E2; }; \
    struct { swizzle<T,4, 0,3,1,3> E0 ## E3 ## E1 ## E3; }; \
    struct { swizzle<T,4, 0,3,2,0> E0 ## E3 ## E2 ## E0; }; \
    struct { swizzle<T,4, 0,3,2,1> E0 ## E3 ## E2 ## E1; }; \
    struct { swizzle<T,4, 0,3,2,2> E0 ## E3 ## E2 ## E2; }; \
    struct { swizzle<T,4, 0,3,2,3> E0 ## E3 ## E2 ## E3; }; \
    struct { swizzle<T,4, 0,3,3,0> E0 ## E3 ## E3 ## E0; }; \
    struct { swizzle<T,4, 0,3,3,1> E0 ## E3 ## E3 ## E1; }; \
    struct { swizzle<T,4, 0,3,3,2> E0 ## E3 ## E3 ## E2; }; \
    struct { swizzle<T,4, 0,3,3,3> E0 ## E3 ## E3 ## E3; }; \
    struct { swizzle<T,4, 1,0,0,0> E1 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,0,0,1> E1 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,0,0,2> E1 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,0,0,3> E1 ## E0 ## E0 ## E3; }; \
    struct { swizzle<T,4, 1,0,1,0> E1 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,0,1,1> E1 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,0,1,2> E1 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,0,1,3> E1 ## E0 ## E1 ## E3; }; \
    struct { swizzle<T,4, 1,0,2,0> E1 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,0,2,1> E1 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,0,2,2> E1 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,0,2,3> E1 ## E0 ## E2 ## E3; }; \
    struct { swizzle<T,4, 1,0,3,0> E1 ## E0 ## E3 ## E0; }; \
    struct { swizzle<T,4, 1,0,3,1> E1 ## E0 ## E3 ## E1; }; \
    struct { swizzle<T,4, 1,0,3,2> E1 ## E0 ## E3 ## E2; }; \
    struct { swizzle<T,4, 1,0,3,3> E1 ## E0 ## E3 ## E3; }; \
    struct { swizzle<T,4, 1,1,0,0> E1 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,1,0,1> E1 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,1,0,2> E1 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,1,0,3> E1 ## E1 ## E0 ## E3; }; \
    struct { swizzle<T,4, 1,1,1,0> E1 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,1,1,1> E1 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,1,1,2> E1 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,1,1,3> E1 ## E1 ## E1 ## E3; }; \
    struct { swizzle<T,4, 1,1,2,0> E1 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,1,2,1> E1 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,1,2,2> E1 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,1,2,3> E1 ## E1 ## E2 ## E3; }; \
    struct { swizzle<T,4, 1,1,3,0> E1 ## E1 ## E3 ## E0; }; \
    struct { swizzle<T,4, 1,1,3,1> E1 ## E1 ## E3 ## E1; }; \
    struct { swizzle<T,4, 1,1,3,2> E1 ## E1 ## E3 ## E2; }; \
    struct { swizzle<T,4, 1,1,3,3> E1 ## E1 ## E3 ## E3; }; \
    struct { swizzle<T,4, 1,2,0,0> E1 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,2,0,1> E1 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,2,0,2> E1 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,2,0,3> E1 ## E2 ## E0 ## E3; }; \
    struct { swizzle<T,4, 1,2,1,0> E1 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,2,1,1> E1 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,2,1,2> E1 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,2,1,3> E1 ## E2 ## E1 ## E3; }; \
    struct { swizzle<T,4, 1,2,2,0> E1 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,2,2,1> E1 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,2,2,2> E1 ## E2 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,2,2,3> E1 ## E2 ## E2 ## E3; }; \
    struct { swizzle<T,4, 1,2,3,0> E1 ## E2 ## E3 ## E0; }; \
    struct { swizzle<T,4, 1,2,3,1> E1 ## E2 ## E3 ## E1; }; \
    struct { swizzle<T,4, 1,2,3,2> E1 ## E2 ## E3 ## E2; }; \
    struct { swizzle<T,4, 1,2,3,3> E1 ## E2 ## E3 ## E3; }; \
    struct { swizzle<T,4, 1,3,0,0> E1 ## E3 ## E0 ## E0; }; \
    struct { swizzle<T,4, 1,3,0,1> E1 ## E3 ## E0 ## E1; }; \
    struct { swizzle<T,4, 1,3,0,2> E1 ## E3 ## E0 ## E2; }; \
    struct { swizzle<T,4, 1,3,0,3> E1 ## E3 ## E0 ## E3; }; \
    struct { swizzle<T,4, 1,3,1,0> E1 ## E3 ## E1 ## E0; }; \
    struct { swizzle<T,4, 1,3,1,1> E1 ## E3 ## E1 ## E1; }; \
    struct { swizzle<T,4, 1,3,1,2> E1 ## E3 ## E1 ## E2; }; \
    struct { swizzle<T,4, 1,3,1,3> E1 ## E3 ## E1 ## E3; }; \
    struct { swizzle<T,4, 1,3,2,0> E1 ## E3 ## E2 ## E0; }; \
    struct { swizzle<T,4, 1,3,2,1> E1 ## E3 ## E2 ## E1; }; \
    struct { swizzle<T,4, 1,3,2,2> E1 ## E3 ## E2 ## E2; }; \
    struct { swizzle<T,4, 1,3,2,3> E1 ## E3 ## E2 ## E3; }; \
    struct { swizzle<T,4, 1,3,3,0> E1 ## E3 ## E3 ## E0; }; \
    struct { swizzle<T,4, 1,3,3,1> E1 ## E3 ## E3 ## E1; }; \
    struct { swizzle<T,4, 1,3,3,2> E1 ## E3 ## E3 ## E2; }; \
    struct { swizzle<T,4, 1,3,3,3> E1 ## E3 ## E3 ## E3; }; \
    struct { swizzle<T,4, 2,0,0,0> E2 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,0,0,1> E2 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,0,0,2> E2 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,0,0,3> E2 ## E0 ## E0 ## E3; }; \
    struct { swizzle<T,4, 2,0,1,0> E2 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,0,1,1> E2 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,0,1,2> E2 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,0,1,3> E2 ## E0 ## E1 ## E3; }; \
    struct { swizzle<T,4, 2,0,2,0> E2 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,0,2,1> E2 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,0,2,2> E2 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,0,2,3> E2 ## E0 ## E2 ## E3; }; \
    struct { swizzle<T,4, 2,0,3,0> E2 ## E0 ## E3 ## E0; }; \
    struct { swizzle<T,4, 2,0,3,1> E2 ## E0 ## E3 ## E1; }; \
    struct { swizzle<T,4, 2,0,3,2> E2 ## E0 ## E3 ## E2; }; \
    struct { swizzle<T,4, 2,0,3,3> E2 ## E0 ## E3 ## E3; }; \
    struct { swizzle<T,4, 2,1,0,0> E2 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,1,0,1> E2 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,1,0,2> E2 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,1,0,3> E2 ## E1 ## E0 ## E3; }; \
    struct { swizzle<T,4, 2,1,1,0> E2 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,1,1,1> E2 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,1,1,2> E2 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,1,1,3> E2 ## E1 ## E1 ## E3; }; \
    struct { swizzle<T,4, 2,1,2,0> E2 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,1,2,1> E2 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,1,2,2> E2 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,1,2,3> E2 ## E1 ## E2 ## E3; }; \
    struct { swizzle<T,4, 2,1,3,0> E2 ## E1 ## E3 ## E0; }; \
    struct { swizzle<T,4, 2,1,3,1> E2 ## E1 ## E3 ## E1; }; \
    struct { swizzle<T,4, 2,1,3,2> E2 ## E1 ## E3 ## E2; }; \
    struct { swizzle<T,4, 2,1,3,3> E2 ## E1 ## E3 ## E3; }; \
    struct { swizzle<T,4, 2,2,0,0> E2 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,2,0,1> E2 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,2,0,2> E2 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,2,0,3> E2 ## E2 ## E0 ## E3; }; \
    struct { swizzle<T,4, 2,2,1,0> E2 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,2,1,1> E2 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,2,1,2> E2 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,2,1,3> E2 ## E2 ## E1 ## E3; }; \
    struct { swizzle<T,4, 2,2,2,0> E2 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,2,2,1> E2 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,2,2,2> E2 ## E2 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,2,2,3> E2 ## E2 ## E2 ## E3; }; \
    struct { swizzle<T,4, 2,2,3,0> E2 ## E2 ## E3 ## E0; }; \
    struct { swizzle<T,4, 2,2,3,1> E2 ## E2 ## E3 ## E1; }; \
    struct { swizzle<T,4, 2,2,3,2> E2 ## E2 ## E3 ## E2; }; \
    struct { swizzle<T,4, 2,2,3,3> E2 ## E2 ## E3 ## E3; }; \
    struct { swizzle<T,4, 2,3,0,0> E2 ## E3 ## E0 ## E0; }; \
    struct { swizzle<T,4, 2,3,0,1> E2 ## E3 ## E0 ## E1; }; \
    struct { swizzle<T,4, 2,3,0,2> E2 ## E3 ## E0 ## E2; }; \
    struct { swizzle<T,4, 2,3,0,3> E2 ## E3 ## E0 ## E3; }; \
    struct { swizzle<T,4, 2,3,1,0> E2 ## E3 ## E1 ## E0; }; \
    struct { swizzle<T,4, 2,3,1,1> E2 ## E3 ## E1 ## E1; }; \
    struct { swizzle<T,4, 2,3,1,2> E2 ## E3 ## E1 ## E2; }; \
    struct { swizzle<T,4, 2,3,1,3> E2 ## E3 ## E1 ## E3; }; \
    struct { swizzle<T,4, 2,3,2,0> E2 ## E3 ## E2 ## E0; }; \
    struct { swizzle<T,4, 2,3,2,1> E2 ## E3 ## E2 ## E1; }; \
    struct { swizzle<T,4, 2,3,2,2> E2 ## E3 ## E2 ## E2; }; \
    struct { swizzle<T,4, 2,3,2,3> E2 ## E3 ## E2 ## E3; }; \
    struct { swizzle<T,4, 2,3,3,0> E2 ## E3 ## E3 ## E0; }; \
    struct { swizzle<T,4, 2,3,3,1> E2 ## E3 ## E3 ## E1; }; \
    struct { swizzle<T,4, 2,3,3,2> E2 ## E3 ## E3 ## E2; }; \
    struct { swizzle<T,4, 2,3,3,3> E2 ## E3 ## E3 ## E3; }; \
    struct { swizzle<T,4, 3,0,0,0> E3 ## E0 ## E0 ## E0; }; \
    struct { swizzle<T,4, 3,0,0,1> E3 ## E0 ## E0 ## E1; }; \
    struct { swizzle<T,4, 3,0,0,2> E3 ## E0 ## E0 ## E2; }; \
    struct { swizzle<T,4, 3,0,0,3> E3 ## E0 ## E0 ## E3; }; \
    struct { swizzle<T,4, 3,0,1,0> E3 ## E0 ## E1 ## E0; }; \
    struct { swizzle<T,4, 3,0,1,1> E3 ## E0 ## E1 ## E1; }; \
    struct { swizzle<T,4, 3,0,1,2> E3 ## E0 ## E1 ## E2; }; \
    struct { swizzle<T,4, 3,0,1,3> E3 ## E0 ## E1 ## E3; }; \
    struct { swizzle<T,4, 3,0,2,0> E3 ## E0 ## E2 ## E0; }; \
    struct { swizzle<T,4, 3,0,2,1> E3 ## E0 ## E2 ## E1; }; \
    struct { swizzle<T,4, 3,0,2,2> E3 ## E0 ## E2 ## E2; }; \
    struct { swizzle<T,4, 3,0,2,3> E3 ## E0 ## E2 ## E3; }; \
    struct { swizzle<T,4, 3,0,3,0> E3 ## E0 ## E3 ## E0; }; \
    struct { swizzle<T,4, 3,0,3,1> E3 ## E0 ## E3 ## E1; }; \
    struct { swizzle<T,4, 3,0,3,2> E3 ## E0 ## E3 ## E2; }; \
    struct { swizzle<T,4, 3,0,3,3> E3 ## E0 ## E3 ## E3; }; \
    struct { swizzle<T,4, 3,1,0,0> E3 ## E1 ## E0 ## E0; }; \
    struct { swizzle<T,4, 3,1,0,1> E3 ## E1 ## E0 ## E1; }; \
    struct { swizzle<T,4, 3,1,0,2> E3 ## E1 ## E0 ## E2; }; \
    struct { swizzle<T,4, 3,1,0,3> E3 ## E1 ## E0 ## E3; }; \
    struct { swizzle<T,4, 3,1,1,0> E3 ## E1 ## E1 ## E0; }; \
    struct { swizzle<T,4, 3,1,1,1> E3 ## E1 ## E1 ## E1; }; \
    struct { swizzle<T,4, 3,1,1,2> E3 ## E1 ## E1 ## E2; }; \
    struct { swizzle<T,4, 3,1,1,3> E3 ## E1 ## E1 ## E3; }; \
    struct { swizzle<T,4, 3,1,2,0> E3 ## E1 ## E2 ## E0; }; \
    struct { swizzle<T,4, 3,1,2,1> E3 ## E1 ## E2 ## E1; }; \
    struct { swizzle<T,4, 3,1,2,2> E3 ## E1 ## E2 ## E2; }; \
    struct { swizzle<T,4, 3,1,2,3> E3 ## E1 ## E2 ## E3; }; \
    struct { swizzle<T,4, 3,1,3,0> E3 ## E1 ## E3 ## E0; }; \
    struct { swizzle<T,4, 3,1,3,1> E3 ## E1 ## E3 ## E1; }; \
    struct { swizzle<T,4, 3,1,3,2> E3 ## E1 ## E3 ## E2; }; \
    struct { swizzle<T,4, 3,1,3,3> E3 ## E1 ## E3 ## E3; }; \
    struct { swizzle<T,4, 3,2,0,0> E3 ## E2 ## E0 ## E0; }; \
    struct { swizzle<T,4, 3,2,0,1> E3 ## E2 ## E0 ## E1; }; \
    struct { swizzle<T,4, 3,2,0,2> E3 ## E2 ## E0 ## E2; }; \
    struct { swizzle<T,4, 3,2,0,3> E3 ## E2 ## E0 ## E3; }; \
    struct { swizzle<T,4, 3,2,1,0> E3 ## E2 ## E1 ## E0; }; \
    struct { swizzle<T,4, 3,2,1,1> E3 ## E2 ## E1 ## E1; }; \
    struct { swizzle<T,4, 3,2,1,2> E3 ## E2 ## E1 ## E2; }; \
    struct { swizzle<T,4, 3,2,1,3> E3 ## E2 ## E1 ## E3; }; \
    struct { swizzle<T,4, 3,2,2,0> E3 ## E2 ## E2 ## E0; }; \
    struct { swizzle<T,4, 3,2,2,1> E3 ## E2 ## E2 ## E1; }; \
    struct { swizzle<T,4, 3,2,2,2> E3 ## E2 ## E2 ## E2; }; \
    struct { swizzle<T,4, 3,2,2,3> E3 ## E2 ## E2 ## E3; }; \
    struct { swizzle<T,4, 3,2,3,0> E3 ## E2 ## E3 ## E0; }; \
    struct { swizzle<T,4, 3,2,3,1> E3 ## E2 ## E3 ## E1; }; \
    struct { swizzle<T,4, 3,2,3,2> E3 ## E2 ## E3 ## E2; }; \
    struct { swizzle<T,4, 3,2,3,3> E3 ## E2 ## E3 ## E3; }; \
    struct { swizzle<T,4, 3,3,0,0> E3 ## E3 ## E0 ## E0; }; \
    struct { swizzle<T,4, 3,3,0,1> E3 ## E3 ## E0 ## E1; }; \
    struct { swizzle<T,4, 3,3,0,2> E3 ## E3 ## E0 ## E2; }; \
    struct { swizzle<T,4, 3,3,0,3> E3 ## E3 ## E0 ## E3; }; \
    struct { swizzle<T,4, 3,3,1,0> E3 ## E3 ## E1 ## E0; }; \
    struct { swizzle<T,4, 3,3,1,1> E3 ## E3 ## E1 ## E1; }; \
    struct { swizzle<T,4, 3,3,1,2> E3 ## E3 ## E1 ## E2; }; \
    struct { swizzle<T,4, 3,3,1,3> E3 ## E3 ## E1 ## E3; }; \
    struct { swizzle<T,4, 3,3,2,0> E3 ## E3 ## E2 ## E0; }; \
    struct { swizzle<T,4, 3,3,2,1> E3 ## E3 ## E2 ## E1; }; \
    struct { swizzle<T,4, 3,3,2,2> E3 ## E3 ## E2 ## E2; }; \
    struct { swizzle<T,4, 3,3,2,3> E3 ## E3 ## E2 ## E3; }; \
    struct { swizzle<T,4, 3,3,3,0> E3 ## E3 ## E3 ## E0; }; \
    struct { swizzle<T,4, 3,3,3,1> E3 ## E3 ## E3 ## E1; }; \
    struct { swizzle<T,4, 3,3,3,2> E3 ## E3 ## E3 ## E2; }; \
    struct { swizzle<T,4, 3,3,3,3> E3 ## E3 ## E3 ## E3; };


    template<typename T>
    struct vector<T, 2>
    {
        constexpr static const int N = 2;
        constexpr static const bool is_simd = storage<T,2>::is_simd;
        typedef T scalar_type;

        union
        {
            struct { T x, y; };
            struct { T r, g; };
            typename storage<T,2>::type data;
            PK_SWIZZLE_MEMBERS_22(T, x, y)
            PK_SWIZZLE_MEMBERS_22(T, r, g)
            PK_SWIZZLE_MEMBERS_23(T, x, y)
            PK_SWIZZLE_MEMBERS_23(T, r, g)
            PK_SWIZZLE_MEMBERS_24(T, x, y)
            PK_SWIZZLE_MEMBERS_24(T, r, g)
        };
        
        constexpr vector() = default;
        constexpr vector(const vector& v) = default;
        constexpr explicit vector(T s) : x(s), y(s) {}
        constexpr explicit vector(const T* ptr) : x(ptr[0]), y(ptr[1]) {}
        constexpr explicit vector(const typename storage<T,2>::type& data) : data(data) {}
        constexpr vector(T _x, T _y) : x(_x), y(_y) {}
        template<typename X, typename Y> constexpr vector(X _x, Y _y) : x(static_cast<T>(_x)), y(static_cast<T>(_y)) {}
        template<typename U> constexpr vector(const vector<U,2>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {}
        template<typename U> constexpr vector(const vector<U,3>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {}
        template<typename U> constexpr vector(const vector<U,4>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {}
        template<int E0, int E1> constexpr vector(const swizzle<T, 2, E0, E1, -1, -2>& s) { *this = s(); }

        constexpr T& operator[](int i) { switch (i) { default: case 0: return x; case 1: return y; } }
        constexpr const T& operator[](int i) const { switch (i) { default: case 0: return x; case 1: return y; }}
        constexpr vector& operator=(const vector& v) = default;
        template<int E0,int E1> constexpr vector& operator=(const swizzle<T,2,E0,E1,-1,-2>& s) { *this = s(); return *this; }
        template<typename U> constexpr vector& operator=(U s) { x = static_cast<T>(s); y = static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator+=(U s) { x += static_cast<T>(s); y += static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator-=(U s) { x -= static_cast<T>(s); y -= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator*=(U s) { x *= static_cast<T>(s); y *= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator/=(U s) { x /= static_cast<T>(s); y /= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator%=(U s) { x %= static_cast<T>(s); y %= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator&=(U s) { x &= static_cast<T>(s); y &= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator|=(U s) { x |= static_cast<T>(s); y |= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator^=(U s) { x ^= static_cast<T>(s); y ^= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator<<=(U s) { x <<= static_cast<T>(s); y <<= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator>>=(U s) { x >>= static_cast<T>(s); y >>= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator=(const vector<U,2>& v) { x = static_cast<T>(v.x); y = static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator+=(const vector<U,2>& v) { x += static_cast<T>(v.x); y += static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator-=(const vector<U,2>& v) { x -= static_cast<T>(v.x); y -= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator*=(const vector<U,2>& v) { x *= static_cast<T>(v.x); y *= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator/=(const vector<U,2>& v) { x /= static_cast<T>(v.x); y /= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator%=(const vector<U,2>& v) { x %= static_cast<T>(v.x); y %= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator&=(const vector<U,2>& v) { x &= static_cast<T>(v.x); y &= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator|=(const vector<U,2>& v) { x |= static_cast<T>(v.x); y |= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator^=(const vector<U,2>& v) { x ^= static_cast<T>(v.x); y ^= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator<<=(const vector<U,2>& v) { x <<= static_cast<T>(v.x); y <<= static_cast<T>(v.y); return *this; }
        template<typename U> constexpr vector& operator>>=(const vector<U,2>& v) { x >>= static_cast<T>(v.x); y >>= static_cast<T>(v.y); return *this; }
        constexpr vector& operator++() { ++x; ++y; return *this; }
        constexpr vector& operator--() { --x; --y; return *this; }
        constexpr vector operator++(int) { vector result(*this); ++* this; return result; }
        constexpr vector operator--(int) { vector result(*this); --* this; return result; }
    };

    template<typename T> constexpr vector<T,2> operator+(const vector<T,2>& v) { return v; }
    template<typename T> constexpr vector<T,2> operator-(const vector<T,2>& v) { return vector<T,2>(-v.x, -v.y); }
    template<typename T> constexpr vector<T,2> operator~(const vector<T,2>& v) { return vector<T,2>(~v.x, ~v.y); }
    template<typename T> constexpr vector<T,2> operator+(const vector<T,2>& v, T s) { return vector<T,2>(v.x + s, v.y + s); }
    template<typename T> constexpr vector<T,2> operator-(const vector<T,2>& v, T s) { return vector<T,2>(v.x - s, v.y - s); }
    template<typename T> constexpr vector<T,2> operator*(const vector<T,2>& v, T s) { return vector<T,2>(v.x * s, v.y * s); }
    template<typename T> constexpr vector<T,2> operator/(const vector<T,2>& v, T s) { return vector<T,2>(v.x / s, v.y / s); }
    template<typename T> constexpr vector<T,2> operator%(const vector<T,2>& v, T s) { return vector<T,2>(v.x % s, v.y % s); }
    template<typename T> constexpr vector<T,2> operator&(const vector<T,2>& v, T s) { return vector<T,2>(v.x & s, v.y & s); }
    template<typename T> constexpr vector<T,2> operator|(const vector<T,2>& v, T s) { return vector<T,2>(v.x | s, v.y | s); }
    template<typename T> constexpr vector<T,2> operator^(const vector<T,2>& v, T s) { return vector<T,2>(v.x ^ s, v.y ^ s); }
    template<typename T> constexpr vector<T,2> operator<<(const vector<T,2>& v, T s) { return vector<T,2>(v.x << s, v.y << s); }
    template<typename T> constexpr vector<T,2> operator>>(const vector<T,2>& v, T s) { return vector<T,2>(v.x >> s, v.y >> s); }
    template<typename T> constexpr vector<T,2> operator+(T s, const vector<T,2>& v) { return vector<T,2>(s + v.x, s + v.y); }
    template<typename T> constexpr vector<T,2> operator-(T s, const vector<T,2>& v) { return vector<T,2>(s - v.x, s - v.y); }
    template<typename T> constexpr vector<T,2> operator*(T s, const vector<T,2>& v) { return vector<T,2>(s * v.x, s * v.y); }
    template<typename T> constexpr vector<T,2> operator/(T s, const vector<T,2>& v) { return vector<T,2>(s / v.x, s / v.y); }
    template<typename T> constexpr vector<T,2> operator%(T s, const vector<T,2>& v) { return vector<T,2>(s % v.x, s % v.y); }
    template<typename T> constexpr vector<T,2> operator&(T s, const vector<T,2>& v) { return vector<T,2>(s & v.x, s & v.y); }
    template<typename T> constexpr vector<T,2> operator|(T s, const vector<T,2>& v) { return vector<T,2>(s | v.x, s | v.y); }
    template<typename T> constexpr vector<T,2> operator^(T s, const vector<T,2>& v) { return vector<T,2>(s ^ v.x, s ^ v.y); }
    template<typename T> constexpr vector<T,2> operator<<(T s, const vector<T,2>& v) { return vector<T,2>(s << v.x, s << v.y); }
    template<typename T> constexpr vector<T,2> operator>>(T s, const vector<T,2>& v) { return vector<T,2>(s >> v.x, s >> v.y); }
    template<typename T> constexpr vector<T,2> operator+(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x + b.x, a.y + b.y); }
    template<typename T> constexpr vector<T,2> operator-(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x - b.x, a.y - b.y); }
    template<typename T> constexpr vector<T,2> operator*(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x * b.x, a.y * b.y); }
    template<typename T> constexpr vector<T,2> operator/(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x / b.x, a.y / b.y); }
    template<typename T> constexpr vector<T,2> operator%(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x % b.x, a.y % b.y); }
    template<typename T> constexpr vector<T,2> operator&(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x & b.x, a.y & b.y); }
    template<typename T> constexpr vector<T,2> operator|(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x | b.x, a.y | b.y); }
    template<typename T> constexpr vector<T,2> operator^(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x ^ b.x, a.y ^ b.y); }
    template<typename T> constexpr vector<T,2> operator<<(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x << b.x, a.y << b.y); }
    template<typename T> constexpr vector<T,2> operator>>(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x >> b.x, a.y >> b.y); }
    template<typename T> constexpr vector<bool,2> operator==(const vector<T,2>& a, const vector<T,2>& b) { return vector<bool,2>(a.x == b.x, a.y == b.y); }
    template<typename T> constexpr vector<bool,2> operator!=(const vector<T,2>& a, const vector<T,2>& b) { return vector<bool,2>(a.x != b.x, a.y != b.y); }
    template<typename T> constexpr vector<bool,2> operator< (const vector<T,2>& a, const vector<T,2>& b) { return vector<bool,2>(a.x <  b.x, a.y <  b.y); }
    template<typename T> constexpr vector<bool,2> operator> (const vector<T,2>& a, const vector<T,2>& b) { return vector<bool,2>(a.x >  b.x, a.y >  b.y); }
    template<typename T> constexpr vector<bool,2> operator<=(const vector<T,2>& a, const vector<T,2>& b) { return vector<bool,2>(a.x <= b.x, a.y <= b.y); }
    template<typename T> constexpr vector<bool,2> operator>=(const vector<T,2>& a, const vector<T,2>& b) { return vector<bool,2>(a.x >= b.x, a.y >= b.y); }
    constexpr inline vector<bool,2> operator!(const vector<bool,2>& v) { return vector<bool,2>(!v.x, !v.y); }
    constexpr inline vector<bool,2> operator&&(const vector<bool,2>& a, const vector<bool,2>& b) { return vector<bool,2>(a.x && b.x, a.y && b.y); }
    constexpr inline vector<bool,2> operator||(const vector<bool,2>& a, const vector<bool,2>& b) { return vector<bool,2>(a.x || b.x, a.y || b.y); }


    template<typename T>
    struct vector<T, 3>
    {
        constexpr static const int N = 3;
        constexpr static const bool is_simd = storage<T,3>::is_simd;
        typedef T scalar_type;

        union
        {
            struct { T x, y, z; };
            struct { T r, g, b; };
            typename storage<T,3>::type data;
            PK_SWIZZLE_MEMBERS_32(T, x, y, z)
            PK_SWIZZLE_MEMBERS_32(T, r, g, b)
            PK_SWIZZLE_MEMBERS_33(T, x, y, z)
            PK_SWIZZLE_MEMBERS_33(T, r, g, b)
            PK_SWIZZLE_MEMBERS_34(T, x, y, z)
            PK_SWIZZLE_MEMBERS_34(T, r, g, b)
        };

        constexpr vector() = default;
        constexpr vector(const vector& v) = default;
        constexpr explicit vector(T s) : x(s), y(s), z(s) {}
        constexpr explicit vector(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}
        constexpr explicit vector(const typename storage<T,3>::type& data) : data(data) {}
        constexpr vector(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
        template<typename X, typename Y, typename Z> constexpr vector(X _x, Y _y, Z _z) : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}
        template<typename XY, typename Z> constexpr vector(const vector<XY,2>& _xy, Z _z) : x(static_cast<T>(_xy.x)), y(static_cast<T>(_xy.y)), z(static_cast<T>(_z)) {}
        template<typename X, typename YZ> constexpr vector(X _x, const vector<YZ,2>& _yz) : x(static_cast<T>(_x)), y(static_cast<T>(_yz.x)), z(static_cast<T>(_yz.y)) {}
        template<typename U> constexpr explicit vector(const vector<U,3>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)) {}
        template<typename U> constexpr explicit vector(const vector<U,4>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)) {}
        template<int E0,int E1,int E2> constexpr vector(const swizzle<T,3,E0,E1,E2,-1>& s) { *this = s(); }
        template<int E0,int E1> constexpr vector(const swizzle<T,2,E0,E1,-1,-2>& v, T s) { *this = vector(v(), s); }
        template<int E0,int E1> constexpr vector(T s, const swizzle<T,2,E0,E1,-1,-2>& v) { *this = vector(s, v()); }

        constexpr T& operator[](int i) { switch (i) { default: case 0: return x; case 1: return y; case 2: return z; } }
        constexpr const T& operator[](int i) const { switch (i) { default: case 0: return x; case 1: return y; case 2: return z; } }
        constexpr vector& operator=(const vector& v) = default;
        template<int E0,int E1,int E2> constexpr vector& operator=(const swizzle<T,3,E0,E1,E2,-1>& s) { *this = s(); return *this; }
        template<typename U> constexpr vector& operator=(U s) { x = static_cast<T>(s); y = static_cast<T>(s); z = static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator+=(U s) { x += static_cast<T>(s); y += static_cast<T>(s); z += static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator-=(U s) { x -= static_cast<T>(s); y -= static_cast<T>(s); z -= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator*=(U s) { x *= static_cast<T>(s); y *= static_cast<T>(s); z *= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator/=(U s) { x /= static_cast<T>(s); y /= static_cast<T>(s); z /= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator%=(U s) { x %= static_cast<T>(s); y %= static_cast<T>(s); z %= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator&=(U s) { x &= static_cast<T>(s); y &= static_cast<T>(s); z &= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator|=(U s) { x |= static_cast<T>(s); y |= static_cast<T>(s); z |= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator^=(U s) { x ^= static_cast<T>(s); y ^= static_cast<T>(s); z ^= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator<<=(U s) { x <<= static_cast<T>(s); y <<= static_cast<T>(s); z <<= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator>>=(U s) { x >>= static_cast<T>(s); y >>= static_cast<T>(s); z >>= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator=(const vector<U,3>& v) { x = static_cast<T>(v.x); y = static_cast<T>(v.y); z = static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator+=(const vector<U,3>& v) { x += static_cast<T>(v.x); y += static_cast<T>(v.y); z += static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator-=(const vector<U,3>& v) { x -= static_cast<T>(v.x); y -= static_cast<T>(v.y); z -= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator*=(const vector<U,3>& v) { x *= static_cast<T>(v.x); y *= static_cast<T>(v.y); z *= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator/=(const vector<U,3>& v) { x /= static_cast<T>(v.x); y /= static_cast<T>(v.y); z /= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator%=(const vector<U,3>& v) { x %= static_cast<T>(v.x); y %= static_cast<T>(v.y); z %= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator&=(const vector<U,3>& v) { x &= static_cast<T>(v.x); y &= static_cast<T>(v.y); z &= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator|=(const vector<U,3>& v) { x |= static_cast<T>(v.x); y |= static_cast<T>(v.y); z |= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator^=(const vector<U,3>& v) { x ^= static_cast<T>(v.x); y ^= static_cast<T>(v.y); z ^= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator<<=(const vector<U,3>& v) { x <<= static_cast<T>(v.x); y <<= static_cast<T>(v.y); z <<= static_cast<T>(v.z); return *this; }
        template<typename U> constexpr vector& operator>>=(const vector<U,3>& v) { x >>= static_cast<T>(v.x); y >>= static_cast<T>(v.y); z >>= static_cast<T>(v.z); return *this; }
        constexpr vector& operator++() { ++x; ++y; ++z; return *this; }
        constexpr vector& operator--() { --x; --y; --z; return *this; }
        constexpr vector operator++(int) { vector result(*this); ++* this; return result; }
        constexpr vector operator--(int) { vector result(*this); --* this; return result; }
    };

    template<typename T> constexpr vector<T,3> operator+(const vector<T,3>& v) { return v; }
    template<typename T> constexpr vector<T,3> operator-(const vector<T,3>& v) { return vector<T,3>(-v.x, -v.y, -v.z); }
    template<typename T> constexpr vector<T,3> operator~(const vector<T,3>& v) { return vector<T,3>(~v.x, ~v.y, ~v.z); }
    template<typename T> constexpr vector<T,3> operator+(const vector<T,3>& v, T s) { return vector<T,3>(v.x + s, v.y + s, v.z + s); }
    template<typename T> constexpr vector<T,3> operator-(const vector<T,3>& v, T s) { return vector<T,3>(v.x - s, v.y - s, v.z - s); }
    template<typename T> constexpr vector<T,3> operator*(const vector<T,3>& v, T s) { return vector<T,3>(v.x * s, v.y * s, v.z * s); }
    template<typename T> constexpr vector<T,3> operator/(const vector<T,3>& v, T s) { return vector<T,3>(v.x / s, v.y / s, v.z / s); }
    template<typename T> constexpr vector<T,3> operator%(const vector<T,3>& v, T s) { return vector<T,3>(v.x % s, v.y % s, v.z % s); }
    template<typename T> constexpr vector<T,3> operator&(const vector<T,3>& v, T s) { return vector<T,3>(v.x & s, v.y & s, v.z & s); }
    template<typename T> constexpr vector<T,3> operator|(const vector<T,3>& v, T s) { return vector<T,3>(v.x | s, v.y | s, v.z | s); }
    template<typename T> constexpr vector<T,3> operator^(const vector<T,3>& v, T s) { return vector<T,3>(v.x ^ s, v.y ^ s, v.z ^ s); }
    template<typename T> constexpr vector<T,3> operator<<(const vector<T,3>& v, T s) { return vector<T,3>(v.x << s, v.y << s, v.z << s); }
    template<typename T> constexpr vector<T,3> operator>>(const vector<T,3>& v, T s) { return vector<T,3>(v.x >> s, v.y >> s, v.z >> s); }
    template<typename T> constexpr vector<T,3> operator+(T s, const vector<T,3>& v) { return vector<T,3>(s + v.x, s + v.y, s + v.z); }
    template<typename T> constexpr vector<T,3> operator-(T s, const vector<T,3>& v) { return vector<T,3>(s - v.x, s - v.y, s - v.z); }
    template<typename T> constexpr vector<T,3> operator*(T s, const vector<T,3>& v) { return vector<T,3>(s * v.x, s * v.y, s * v.z); }
    template<typename T> constexpr vector<T,3> operator/(T s, const vector<T,3>& v) { return vector<T,3>(s / v.x, s / v.y, s / v.z); }
    template<typename T> constexpr vector<T,3> operator%(T s, const vector<T,3>& v) { return vector<T,3>(s % v.x, s % v.y, s % v.z); }
    template<typename T> constexpr vector<T,3> operator&(T s, const vector<T,3>& v) { return vector<T,3>(s & v.x, s & v.y, s & v.z); }
    template<typename T> constexpr vector<T,3> operator|(T s, const vector<T,3>& v) { return vector<T,3>(s | v.x, s | v.y, s | v.z); }
    template<typename T> constexpr vector<T,3> operator^(T s, const vector<T,3>& v) { return vector<T,3>(s ^ v.x, s ^ v.y, s ^ v.z); }
    template<typename T> constexpr vector<T,3> operator<<(T s, const vector<T,3>& v) { return vector<T,3>(s << v.x, s << v.y, s << v.z); }
    template<typename T> constexpr vector<T,3> operator>>(T s, const vector<T,3>& v) { return vector<T,3>(s >> v.x, s >> v.y, s >> v.z); }
    template<typename T> constexpr vector<T,3> operator+(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x + b.x, a.y + b.y, a.z + b.z); }
    template<typename T> constexpr vector<T,3> operator-(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x - b.x, a.y - b.y, a.z - b.z); }
    template<typename T> constexpr vector<T,3> operator*(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x * b.x, a.y * b.y, a.z * b.z); }
    template<typename T> constexpr vector<T,3> operator/(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x / b.x, a.y / b.y, a.z / b.z); }
    template<typename T> constexpr vector<T,3> operator%(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x % b.x, a.y % b.y, a.z % b.z); }
    template<typename T> constexpr vector<T,3> operator&(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x & b.x, a.y & b.y, a.z & b.z); }
    template<typename T> constexpr vector<T,3> operator|(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x | b.x, a.y | b.y, a.z | b.z); }
    template<typename T> constexpr vector<T,3> operator^(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x ^ b.x, a.y ^ b.y, a.z ^ b.z); }
    template<typename T> constexpr vector<T,3> operator<<(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x << b.x, a.y << b.y, a.z << b.z); }
    template<typename T> constexpr vector<T,3> operator>>(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.x >> b.x, a.y >> b.y, a.z >> b.z); }
    template<typename T> constexpr vector<bool,3> operator==(const vector<T,3>& a, const vector<T,3>& b) { return vector<bool,3>(a.x == b.x, a.y == b.y, a.z == b.z); }
    template<typename T> constexpr vector<bool,3> operator!=(const vector<T,3>& a, const vector<T,3>& b) { return vector<bool,3>(a.x != b.x, a.y != b.y, a.z != b.z); }
    template<typename T> constexpr vector<bool,3> operator< (const vector<T,3>& a, const vector<T,3>& b) { return vector<bool,3>(a.x <  b.x, a.y <  b.y, a.z <  b.z); }
    template<typename T> constexpr vector<bool,3> operator> (const vector<T,3>& a, const vector<T,3>& b) { return vector<bool,3>(a.x >  b.x, a.y >  b.y, a.z >  b.z); }
    template<typename T> constexpr vector<bool,3> operator<=(const vector<T,3>& a, const vector<T,3>& b) { return vector<bool,3>(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
    template<typename T> constexpr vector<bool,3> operator>=(const vector<T,3>& a, const vector<T,3>& b) { return vector<bool,3>(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
    constexpr inline vector<bool,3> operator!(const vector<bool,3>& v) { return vector<bool,3>(!v.x, !v.y, !v.z); }
    constexpr inline vector<bool,3> operator&&(const vector<bool,3>& a, const vector<bool,3>& b) { return vector<bool,3>(a.x && b.x, a.y && b.y, a.z && b.z); }
    constexpr inline vector<bool,3> operator||(const vector<bool,3>& a, const vector<bool,3>& b) { return vector<bool,3>(a.x || b.x, a.y || b.y, a.z || b.z); }


    template<typename T>
    struct vector<T, 4>
    {
        constexpr static const int N = 4;
        constexpr static const bool is_simd = storage<T,4>::is_simd;
        typedef T scalar_type;

        union
        {
            struct { T x, y, z, w; };
            struct { T r, g, b, a; };
            typename storage<T,4>::type data; 
            PK_SWIZZLE_MEMBERS_42(T, x, y, z, w)
            PK_SWIZZLE_MEMBERS_42(T, r, g, b, a)
            PK_SWIZZLE_MEMBERS_43(T, x, y, z, w)
            PK_SWIZZLE_MEMBERS_43(T, r, g, b, a)
            PK_SWIZZLE_MEMBERS_44(T, x, y, z, w)
            PK_SWIZZLE_MEMBERS_44(T, r, g, b, a)
        };

        constexpr vector() = default;
        constexpr vector(const vector<T,4>& v) = default;
        constexpr explicit vector(T s) : x(s), y(s), z(s), w(s) {}
        constexpr explicit vector(const T* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}
        constexpr explicit vector(const typename storage<T,4>::type& data) : data(data) {}
        constexpr vector(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
        template<typename X, typename Y, typename Z, typename W> constexpr vector(X _x, Y _y, Z _z, W _w) : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)), w(static_cast<T>(_w)) {}
        template<typename XY, typename Z, typename W> constexpr vector(const vector<XY,2>& _xy, Z _z, W _w) : x(static_cast<T>(_xy.x)), y(static_cast<T>(_xy.y)), z(static_cast<T>(_z)), w(static_cast<T>(_w)) {}
        template<typename X, typename YZ, typename W> constexpr vector(X _x, const vector<YZ,2>& _yz, W _w) : x(static_cast<T>(_x)), y(static_cast<T>(_yz.x)), z(static_cast<T>(_yz.y)), w(static_cast<T>(_w)) {}
        template<typename X, typename Y, typename ZW> constexpr vector(X _x, Y _y, const vector<ZW,2>& _zw) : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_zw.x)), w(static_cast<T>(_zw.y)) {}
        template<typename XYZ, typename W> constexpr vector(const vector<XYZ,3>& _xyz, W _w) : x(static_cast<T>(_xyz.x)), y(static_cast<T>(_xyz.y)), z(static_cast<T>(_xyz.z)), w(static_cast<T>(_w)) {}
        template<typename X, typename YZW> constexpr vector(X _x, const vector<YZW,3>& _yzw) : x(static_cast<T>(_x)), y(static_cast<T>(_yzw.x)), z(static_cast<T>(_yzw.y)), w(static_cast<T>(_yzw.z)) {}
        template<typename XY, typename ZW> constexpr vector(const vector<XY,2>& _xy, const vector<ZW,2>& _zw) : x(static_cast<T>(_xy.x)), y(static_cast<T>(_xy.y)), z(static_cast<T>(_zw.x)), w(static_cast<T>(_zw.y)) {}
        template<typename U> constexpr explicit vector(const vector<U,4>& v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(v.w)) {}
        template<int E0,int E1,int E2,int E3> constexpr vector(const swizzle<T,4,E0,E1,E2,E3>& s) { *this = s(); }
        template<int E0,int E1,int F0,int F1> constexpr vector(const swizzle<T,2,E0,E1,-1,-2>& v, const swizzle<T,2, F0, F1, -1, -2>& u) { *this = vector<T,4>(v(), u()); }
        template<int E0,int E1> constexpr vector(T x, T y, const swizzle<T,2,E0,E1,-1,-2>& v) { *this = vector<T,4>(x, y, v()); }
        template<int E0,int E1> constexpr vector(T x, const swizzle<T,2,E0,E1,-1,-2>& v, T w) { *this = vector<T,4>(x, v(), w); }
        template<int E0,int E1> constexpr vector(const swizzle<T,2, E0, E1, -1, -2>& v, T z, T w) { *this = vector<T,4>(v(), z, w); }
        template<int E0,int E1,int E2> constexpr vector(const swizzle<T,3,E0,E1,E2,-1>& v, T w) { *this = vector<T,4>(v(), w); }
        template<int E0,int E1,int E2> constexpr vector(T x, const swizzle<T,3,E0,E1,E2,-1>& v) { *this = vector<T,4>(x, v()); }

        constexpr T& operator[](int i) { switch (i) { default: case 0: return x; case 1: return y; case 2: return z; case 3: return w; } }
        constexpr const T& operator[](int i) const { switch (i) { default: case 0: return x; case 1: return y; case 2: return z; case 3: return w; } }
        constexpr vector& operator=(const vector& v) = default;
        template<int E0,int E1,int E2,int E3> constexpr vector& operator=(const swizzle<T,4,E0,E1,E2,E3>& s) { *this = s(); return *this; }
        template<typename U> constexpr vector& operator=(U s) { x = static_cast<T>(s); y = static_cast<T>(s); z = static_cast<T>(s); w = static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator+=(U s) { x += static_cast<T>(s); y += static_cast<T>(s); z += static_cast<T>(s); w += static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator-=(U s) { x -= static_cast<T>(s); y -= static_cast<T>(s); z -= static_cast<T>(s); w -= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator*=(U s) { x *= static_cast<T>(s); y *= static_cast<T>(s); z *= static_cast<T>(s); w *= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator/=(U s) { x /= static_cast<T>(s); y /= static_cast<T>(s); z /= static_cast<T>(s); w /= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator%=(U s) { x %= static_cast<T>(s); y %= static_cast<T>(s); z %= static_cast<T>(s); w %= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator&=(U s) { x &= static_cast<T>(s); y &= static_cast<T>(s); z &= static_cast<T>(s); w &= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator|=(U s) { x |= static_cast<T>(s); y |= static_cast<T>(s); z |= static_cast<T>(s); w |= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator^=(U s) { x ^= static_cast<T>(s); y ^= static_cast<T>(s); z ^= static_cast<T>(s); w ^= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator<<=(U s) { x <<= static_cast<T>(s); y <<= static_cast<T>(s); z <<= static_cast<T>(s); w <<= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator>>=(U s) { x >>= static_cast<T>(s); y >>= static_cast<T>(s); z >>= static_cast<T>(s); w >>= static_cast<T>(s); return *this; }
        template<typename U> constexpr vector& operator=(const vector<U,4>& v) { x = static_cast<T>(v.x); y = static_cast<T>(v.y); z = static_cast<T>(v.z); w = static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator+=(const vector<U,4>& v) { x += static_cast<T>(v.x); y += static_cast<T>(v.y); z += static_cast<T>(v.z); w += static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator-=(const vector<U,4>& v) { x -= static_cast<T>(v.x); y -= static_cast<T>(v.y); z -= static_cast<T>(v.z); w -= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator*=(const vector<U,4>& v) { x *= static_cast<T>(v.x); y *= static_cast<T>(v.y); z *= static_cast<T>(v.z); w *= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator/=(const vector<U,4>& v) { x /= static_cast<T>(v.x); y /= static_cast<T>(v.y); z /= static_cast<T>(v.z); w /= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator%=(const vector<U,4>& v) { x %= static_cast<T>(v.x); y %= static_cast<T>(v.y); z %= static_cast<T>(v.z); w %= static_cast<T>(v.w); return *this; }  
        template<typename U> constexpr vector& operator&=(const vector<U,4>& v) { x &= static_cast<T>(v.x); y &= static_cast<T>(v.y); z &= static_cast<T>(v.z); w &= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator|=(const vector<U,4>& v) { x |= static_cast<T>(v.x); y |= static_cast<T>(v.y); z |= static_cast<T>(v.z); w |= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator^=(const vector<U,4>& v) { x ^= static_cast<T>(v.x); y ^= static_cast<T>(v.y); z ^= static_cast<T>(v.z); w ^= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator<<=(const vector<U,4>& v) { x <<= static_cast<T>(v.x); y <<= static_cast<T>(v.y); z <<= static_cast<T>(v.z); w <<= static_cast<T>(v.w); return *this; }
        template<typename U> constexpr vector& operator>>=(const vector<U,4>& v) { x >>= static_cast<T>(v.x); y >>= static_cast<T>(v.y); z >>= static_cast<T>(v.z); w >>= static_cast<T>(v.w); return *this; }
        constexpr vector& operator++() { ++x; ++y; ++z; ++w; return *this; }
        constexpr vector& operator--() { --x; --y; --z; --w; return *this; }
        constexpr vector operator++(int) { vector result(*this); ++* this; return result; }
        constexpr vector operator--(int) { vector result(*this); --* this; return result; }
    };

    template<typename T> constexpr vector<T,4> operator+(const vector<T,4>& v) { return v; }
    template<typename T> constexpr vector<T,4> operator-(const vector<T,4>& v) { return vector<T,4>(-v.x,-v.y,-v.z,-v.w); }
    template<typename T> constexpr vector<T,4> operator~(const vector<T,4>& v) { return vector<T, 4>(~v.x, ~v.y, ~v.z, ~v.w); }
    template<typename T> constexpr vector<T,4> operator+(const vector<T,4>& v, T s) { return vector<T,4>(v.x + s, v.y + s, v.z + s, v.w + s); }
    template<typename T> constexpr vector<T,4> operator-(const vector<T,4>& v, T s) { return vector<T,4>(v.x - s, v.y - s, v.z - s, v.w - s); }
    template<typename T> constexpr vector<T,4> operator*(const vector<T,4>& v, T s) { return vector<T,4>(v.x * s, v.y * s, v.z * s, v.w * s); }
    template<typename T> constexpr vector<T,4> operator/(const vector<T,4>& v, T s) { return vector<T,4>(v.x / s, v.y / s, v.z / s, v.w / s); }
    template<typename T> constexpr vector<T,4> operator%(const vector<T,4>& v, T s) { return vector<T,4>(v.x % s, v.y % s, v.z % s, v.w % s); }
    template<typename T> constexpr vector<T,4> operator&(const vector<T,4>& v, T s) { return vector<T,4>(v.x & s, v.y & s, v.z & s, v.w & s); }
    template<typename T> constexpr vector<T,4> operator|(const vector<T,4>& v, T s) { return vector<T,4>(v.x | s, v.y | s, v.z | s, v.w | s); }
    template<typename T> constexpr vector<T,4> operator^(const vector<T,4>& v, T s) { return vector<T,4>(v.x ^ s, v.y ^ s, v.z ^ s, v.w ^ s); }
    template<typename T> constexpr vector<T,4> operator<<(const vector<T,4>& v, T s) { return vector<T,4>(v.x << s, v.y << s, v.z << s, v.w << s); }
    template<typename T> constexpr vector<T,4> operator>>(const vector<T,4>& v, T s) { return vector<T,4>(v.x >> s, v.y >> s, v.z >> s, v.w >> s); }
    template<typename T> constexpr vector<T,4> operator+(T s, const vector<T,4>& v) { return vector<T,4>(s + v.x, s + v.y, s + v.z, s + v.w); }
    template<typename T> constexpr vector<T,4> operator-(T s, const vector<T,4>& v) { return vector<T,4>(s - v.x, s - v.y, s - v.z, s - v.w); }
    template<typename T> constexpr vector<T,4> operator*(T s, const vector<T,4>& v) { return vector<T,4>(s * v.x, s * v.y, s * v.z, s * v.w); }
    template<typename T> constexpr vector<T,4> operator/(T s, const vector<T,4>& v) { return vector<T,4>(s / v.x, s / v.y, s / v.z, s / v.w); }
    template<typename T> constexpr vector<T,4> operator%(T s, const vector<T,4>& v) { return vector<T,4>(s % v.x, s % v.y, s % v.z, s % v.w); }
    template<typename T> constexpr vector<T,4> operator&(T s, const vector<T,4>& v) { return vector<T,4>(s & v.x, s & v.y, s & v.z, s & v.w); }
    template<typename T> constexpr vector<T,4> operator|(T s, const vector<T,4>& v) { return vector<T,4>(s | v.x, s | v.y, s | v.z, s | v.w); }
    template<typename T> constexpr vector<T,4> operator^(T s, const vector<T,4>& v) { return vector<T,4>(s ^ v.x, s ^ v.y, s ^ v.z, s ^ v.w); }
    template<typename T> constexpr vector<T,4> operator<<(T s, const vector<T,4>& v) { return vector<T,4>(s << v.x, s << v.y, s << v.z, s << v.w); }
    template<typename T> constexpr vector<T,4> operator>>(T s, const vector<T,4>& v) { return vector<T,4>(s >> v.x, s >> v.y, s >> v.z, s >> v.w); }
    template<typename T> constexpr vector<T,4> operator+(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
    template<typename T> constexpr vector<T,4> operator-(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
    template<typename T> constexpr vector<T,4> operator*(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
    template<typename T> constexpr vector<T,4> operator/(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }
    template<typename T> constexpr vector<T,4> operator%(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w); }
    template<typename T> constexpr vector<T,4> operator&(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w); }
    template<typename T> constexpr vector<T,4> operator|(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w); }
    template<typename T> constexpr vector<T,4> operator^(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w); }
    template<typename T> constexpr vector<T,4> operator<<(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x << b.x, a.y << b.y, a.z << b.z, a.w << b.w); }
    template<typename T> constexpr vector<T,4> operator>>(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(a.x >> b.x, a.y >> b.y, a.z >> b.z, a.w >> b.w); }
    template<typename T> constexpr vector<bool,4> operator==(const vector<T,4>& a, const vector<T,4>& b) { return vector<bool,4>(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }
    template<typename T> constexpr vector<bool,4> operator!=(const vector<T,4>& a, const vector<T,4>& b) { return vector<bool,4>(a.x != b.x, a.y != b.y, a.z != b.z, a.w != b.w); }
    template<typename T> constexpr vector<bool,4> operator< (const vector<T,4>& a, const vector<T,4>& b) { return vector<bool,4>(a.x <  b.x, a.y <  b.y, a.z <  b.z, a.w <  b.w); }
    template<typename T> constexpr vector<bool,4> operator> (const vector<T,4>& a, const vector<T,4>& b) { return vector<bool,4>(a.x >  b.x, a.y >  b.y, a.z >  b.z, a.w >  b.w); }
    template<typename T> constexpr vector<bool,4> operator<=(const vector<T,4>& a, const vector<T,4>& b) { return vector<bool,4>(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
    template<typename T> constexpr vector<bool,4> operator>=(const vector<T,4>& a, const vector<T,4>& b) { return vector<bool,4>(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }
    constexpr inline vector<bool,4> operator!(const vector<bool,4>& v) { return vector<bool,4>(!v.x, !v.y, !v.z, !v.w); }
    constexpr inline vector<bool,4> operator&&(const vector<bool,4>& a, const vector<bool,4>& b) { return vector<bool,4>(a.x && b.x, a.y && b.y, a.z && b.z, a.w && b.w); }
    constexpr inline vector<bool,4> operator||(const vector<bool,4>& a, const vector<bool,4>& b) { return vector<bool,4>(a.x || b.x, a.y || b.y, a.z || b.z, a.w || b.w); }


    template<typename T>
    struct matrix<T, 2, 2>
    {
        typedef vector<T,2> col_type;
        typedef vector<T,2> row_type;
        col_type columns[2];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0), col_type(0, s) } {}
        constexpr matrix(T x0, T y0, T x1, T y1) : columns{ col_type(x0, y0), col_type(x1, y1) } {}
        constexpr matrix(const col_type& v0, const col_type& v1) : columns{ v0, v1 } {}
        template<typename X0, typename Y0, typename X1, typename Y1> constexpr matrix(X0 x0, Y0 y0, X1 x1, Y1 y1) : columns{ col_type(x0, y0), col_type(x1, y1) } {}
        template<typename V0, typename V1> constexpr matrix(const vector<V0, 2>& v0, const vector<V1, 2>& v1) : columns{ col_type(v0), col_type(v1) } {}
        template<typename U> constexpr explicit matrix(const matrix<U, 2, 2> & m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,2,2>& m) { columns[0] = m[0]; columns[1] = m[1]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,2,2>& m) { columns[0] += m[0]; columns[1] += m[1]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,2,2>& m) { columns[0] -= m[0]; columns[1] -= m[1]; return *this; }
        template<typename U> constexpr matrix& operator*=(const matrix<U,2,2>& m) { return (*this = *this * m); }
        template<typename U> matrix& operator/=(const matrix<U,2,2>& m) { return *this *= inverse(m); }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };
    
    template<typename T> constexpr typename matrix<T,2,2>::col_type operator*(const matrix<T,2,2>& m, const typename matrix<T,2,2>::row_type& v)
    { 
        return typename matrix<T,2,2>::col_type(
            m[0][0] * v.x + m[1][0] * v.y, 
            m[0][1] * v.x + m[1][1] * v.y); 
    }

    template<typename T> constexpr typename matrix<T,2,2>::row_type operator*(const typename matrix<T,2,2>::col_type& v, const matrix<T,2,2>& m)
    { 
        return typename matrix<T,2,2>::row_type(
            v.x * m[0][0] + v.y * m[0][1], 
            v.x * m[1][0] + v.y * m[1][1]); 
    }

    template<typename T> constexpr matrix<T,2,2> operator*(const matrix<T,2,2>& m1, const matrix<T,2,2>& m2)
    { 
            return matrix<T,2,2>( 
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1]);
    }

    template<typename T> constexpr matrix<T,3,2> operator*(const matrix<T,2,2>& m1, const matrix<T,3,2>& m2)
    {
        return matrix<T,3,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1]);
    }

    template<typename T> constexpr matrix<T,4,2> operator*(const matrix<T,2,2>& m1, const matrix<T,4,2>& m2)
    {
        return matrix<T,4,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1]);
    }

    template<typename T> constexpr matrix<T,2,2> operator+(const matrix<T,2,2>& m) { return m; }
    template<typename T> constexpr matrix<T,2,2> operator-(const matrix<T,2,2>& m) { return matrix<T,2,2>(-m[0], -m[1]); }
    template<typename T> constexpr matrix<T,2,2> operator+(const matrix<T,2,2>& m, T s) { return matrix<T,2,2>(m[0] + s, m[1] + s); }
    template<typename T> constexpr matrix<T,2,2> operator-(const matrix<T,2,2>& m, T s) { return matrix<T,2,2>(m[0] - s, m[1] - s); }
    template<typename T> constexpr matrix<T,2,2> operator*(const matrix<T,2,2>& m, T s) { return matrix<T,2,2>(m[0] * s, m[1] * s); }
    template<typename T> constexpr matrix<T,2,2> operator/(const matrix<T,2,2>& m, T s) { return matrix<T,2,2>(m[0] / s, m[1] / s);}
    template<typename T> constexpr matrix<T,2,2> operator+(T s, const matrix<T,2,2>& m) { return matrix<T,2,2>(m[0] + s, m[1] + s); }
    template<typename T> constexpr matrix<T,2,2> operator-(T s, const matrix<T,2,2>& m) { return matrix<T,2,2>(s - m[0], s - m[1]); }
    template<typename T> constexpr matrix<T,2,2> operator*(T s, const matrix<T,2,2>& m) { return matrix<T,2,2>(m[0] * s, m[1] * s); }
    template<typename T> constexpr matrix<T,2,2> operator/(T s, const matrix<T,2,2>& m) { return matrix<T,2,2>(s / m[0], s / m[1]); }
    template<typename T> constexpr matrix<T,2,2> operator+(const matrix<T,2,2>& m1, const matrix<T,2,2>& m2) { return matrix<T,2,2>(m1[0] + m2[0], m1[1] + m2[1]); }
    template<typename T> constexpr matrix<T,2,2> operator-(const matrix<T,2,2>& m1, const matrix<T,2,2>& m2) { return matrix<T,2,2>(m1[0] - m2[0], m1[1] - m2[1]); }
    template<typename T> constexpr matrix<T,2,2> operator/(const matrix<T,2,2>& m1, const matrix<T,2,2>& m2) { matrix<T,2,2> m1_copy(m1); return m1_copy /= m2; }
    template<typename T> typename matrix<T,2,2>::col_type operator/(const matrix<T,2,2>& m, const typename matrix<T,2,2>::row_type& v) { return inverse(m) * v; }
    template<typename T> typename matrix<T,2,2>::row_type operator/(const typename matrix<T,2,2>::col_type& v, const matrix<T,2,2>& m) { return v * inverse(m); }
    template<typename T> constexpr bool operator==(const matrix<T,2,2>& m1, const matrix<T,2,2>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]); }
    template<typename T> constexpr bool operator!=(const matrix<T,2,2>& m1, const matrix<T,2,2>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]); }


    template<typename T>
    struct matrix<T,2,3>
    {
        typedef vector<T,3> col_type;
        typedef vector<T,2> row_type;
        col_type columns[2];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0, 0), col_type(0, s, 0) } { }
        constexpr matrix(T x0, T y0, T z0, T x1, T y1, T z1 ) : columns{ col_type(x0, y0, z0), col_type(x1, y1, z1) } {}
        constexpr matrix(const col_type& v0, const col_type& v1) : columns{ col_type(v0), col_type(v1) } {}
        template<typename X0, typename Y0, typename Z0, typename X1, typename Y1, typename Z1> constexpr matrix(X0 x0, Y0 y0, Z0 z0, X1 x1, Y1 y1, Z1 z1) : columns{ col_type(x0, y0, z0), col_type(x1, y1, z1) } {}
        template<typename V0, typename V1> constexpr matrix(const vector<V0,3>& v0, const vector<V1,3>& v1) : columns{ col_type(v0), col_type(v1) } {}
        template<typename U> constexpr explicit matrix(const matrix<U,2,3>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,2,3>& m) { columns[0] = m[0]; columns[1] = m[1]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,2,3>& m) { columns[0] += m[0]; columns[1] += m[1]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,2,3>& m) { columns[0] -= m[0]; columns[1] -= m[1]; return *this; }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };
    
    template<typename T> constexpr typename matrix<T,2,3>::col_type operator*(const matrix<T,2,3>& m, const typename matrix<T,2,3>::row_type& v)
    { 
        return typename matrix<T,2,3>::col_type(
            m[0][0] * v.x + m[1][0] * v.y, 
            m[0][1] * v.x + m[1][1] * v.y, 
            m[0][2] * v.x + m[1][2] * v.y); 
    }
    
    template<typename T> constexpr typename matrix<T,2,3>::row_type operator*(const typename matrix<T,2,3>::col_type& v, const matrix<T,2,3>& m)
    { 
        return typename matrix<T,2,3>::row_type(
            v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2], 
            v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2]); 
    }
    
    template<typename T> constexpr matrix<T,2,3>operator*(const matrix<T,2,3>& m1, const matrix<T,2, 2>& m2)
    {
        return matrix<T,2,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1]);
    }

    template<typename T> constexpr matrix<T,3,3> operator*(const matrix<T,2,3>& m1, const matrix<T,3,2>& m2)
    {
        return matrix<T,3,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1]);
    }

    template<typename T> constexpr matrix<T,4,3> operator*(const matrix<T,2,3>& m1, const matrix<T,4,2>& m2)
    {
        return matrix<T,4,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1],
            m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1]);
    }

    template<typename T> constexpr matrix<T,2,3> operator+(const matrix<T,2,3>& m) { return m; }
    template<typename T> constexpr matrix<T,2,3> operator-(const matrix<T,2,3>& m) { return matrix<T,2,3>(-m[0], -m[1]); }
    template<typename T> constexpr matrix<T,2,3> operator+(const matrix<T,2,3>& m, T s) { return matrix<T,2,3>(m[0] + s, m[1] + s); }
    template<typename T> constexpr matrix<T,2,3> operator-(const matrix<T,2,3>& m, T s) { return matrix<T,2,3>(m[0] - s, m[1] - s); }
    template<typename T> constexpr matrix<T,2,3> operator*(const matrix<T,2,3>& m, T s) { return matrix<T,2,3>(m[0] * s, m[1] * s); }
    template<typename T> constexpr matrix<T,2,3> operator/(const matrix<T,2,3>& m, T s) { return matrix<T,2,3>(m[0] / s, m[1] / s); }
    template<typename T> constexpr matrix<T,2,3> operator*(T s, const matrix<T,2,3>& m) { return matrix<T,2,3>(m[0] * s, m[1] * s); }
    template<typename T> constexpr matrix<T,2,3> operator/(T s, const matrix<T,2,3>& m) { return matrix<T,2,3>(s / m[0], s / m[1]); }
    template<typename T> constexpr matrix<T,2,3> operator+(const matrix<T,2,3>& m1, const matrix<T,2,3>& m2) { return matrix<T,2,3>(m1[0] + m2[0], m1[1] + m2[1]); }
    template<typename T> constexpr matrix<T,2,3> operator-(const matrix<T,2,3>& m1, const matrix<T,2,3>& m2) { return matrix<T,2,3>(m1[0] - m2[0], m1[1] - m2[1]); }
    template<typename T> constexpr bool operator==(const matrix<T,2,3>& m1, const matrix<T,2,3>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]); }
    template<typename T> constexpr bool operator!=(const matrix<T,2,3>& m1, const matrix<T,2,3>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]); }


    template<typename T>
    struct matrix<T,2,4>
    {
        typedef vector<T,4> col_type;
        typedef vector<T,2> row_type;
        col_type columns[2];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0, 0, 0), col_type(0, s, 0, 0) } {}
        constexpr matrix(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1) : columns{ col_type(x0, y0, z0, w0), col_type(x1, y1, z1, w1) } {}
        constexpr matrix(const col_type& v0, const col_type& v1) : columns{ col_type(v0), col_type(v1) } {}
        template<typename X0, typename Y0, typename Z0, typename W0, typename X1, typename Y1, typename Z1, typename W1> constexpr matrix(X0 x0, Y0 y0, Z0 z0, W0 w0, X1 x1, Y1 y1, Z1 z1, W1 w1) : columns{col_type(x0, y0, z0, w0), col_type(x1, y1, z1, w1) } {}
        template<typename V0, typename V1> constexpr matrix(const vector<V0,4>& v0, const vector<V1,4>& v1) : columns{ col_type(v0), col_type(v1) } {}
        template<typename U> constexpr explicit matrix(const matrix<U,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,2,4>& m) { columns[0] = m[0]; columns[1] = m[1]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,2,4>& m) { columns[0] += m[0]; columns[1] += m[1]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,2,4>& m) { columns[0] -= m[0]; columns[1] -= m[1]; return *this; }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };
    
    template<typename T> constexpr typename matrix<T,2,4>::col_type operator*(const matrix<T,2,4>& m, const typename matrix<T,2,4>::row_type& v)
    { 
        return typename matrix<T,2,4>::col_type(
            m[0][0] * v.x + m[1][0] * v.y, 
            m[0][1] * v.x + m[1][1] * v.y, 
            m[0][2] * v.x + m[1][2] * v.y, 
            m[0][3] * v.x + m[1][3] * v.y); 
    }

    template<typename T> constexpr typename matrix<T,2,4>::row_type operator*(const typename matrix<T,2,4>::col_type& v, const matrix<T,2,4>& m)
    {
        return typename matrix<T,2,4>::row_type(
            v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + v.w * m[0][3], 
            v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + v.w * m[1][3]);
    }

    template<typename T> constexpr matrix<T,4,4> operator*(const matrix<T,2,4>& m1, const matrix<T,4,2>& m2)
    {
        return matrix<T,4,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1],
            m1[0][3] * m2[2][0] + m1[1][3] * m2[2][1],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1],
            m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1],
            m1[0][3] * m2[3][0] + m1[1][3] * m2[3][1]);
    }

    template<typename T> constexpr matrix<T,2,4> operator*(const matrix<T,2,4>& m1, const matrix<T,2,2>& m2)
    {
        return matrix<T,2,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1]);
    }

    template<typename T> constexpr matrix<T,3,4> operator*(const matrix<T,2,4>& m1, const matrix<T,3,2>& m2)
    {
        return matrix<T,3,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1],
            m1[0][3] * m2[2][0] + m1[1][3] * m2[2][1]);
    }

    template<typename T> constexpr matrix<T,2,4> operator+(const matrix<T,2,4>& m) { return m; }
    template<typename T> constexpr matrix<T,2,4> operator-(const matrix<T,2,4>& m) { return matrix<T,2,4>(-m[0], -m[1]); }
    template<typename T> constexpr matrix<T,2,4> operator+(const matrix<T,2,4>& m, T s) { return matrix<T,2,4>(m[0] + s, m[1] + s); }
    template<typename T> constexpr matrix<T,2,4> operator-(const matrix<T,2,4>& m, T s) { return matrix<T,2,4>(m[0] - s, m[1] - s); }
    template<typename T> constexpr matrix<T,2,4> operator*(const matrix<T,2,4>& m, T s) { return matrix<T,2,4>(m[0] * s, m[1] * s); }
    template<typename T> constexpr matrix<T,2,4> operator/(const matrix<T,2,4>& m, T s) { return matrix<T,2,4>(m[0] / s, m[1] / s); }
    template<typename T> constexpr matrix<T,2,4> operator*(T s, const matrix<T,2,4>& m) { return matrix<T,2,4>(m[0] * s, m[1] * s); }
    template<typename T> constexpr matrix<T,2,4> operator/(T s, const matrix<T,2,4>& m) { return matrix<T,2,4>(s / m[0], s / m[1]); }
    template<typename T> constexpr matrix<T,2,4> operator+(const matrix<T,2,4>& m1, const matrix<T,2,4>& m2) { return matrix<T,2,4>(m1[0] + m2[0], m1[1] + m2[1]); }
    template<typename T> constexpr matrix<T,2,4> operator-(const matrix<T,2,4>& m1, const matrix<T,2,4>& m2) { return matrix<T,2,4>(m1[0] - m2[0], m1[1] - m2[1]); }
    template<typename T> constexpr bool operator==(const matrix<T,2,4>& m1, const matrix<T,2,4>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]); }
    template<typename T> constexpr bool operator!=(const matrix<T,2,4>& m1, const matrix<T,2,4>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]); }


    template<typename T>
    struct matrix<T,3,2>
    {
        typedef vector<T,2> col_type;
        typedef vector<T,3> row_type;
        col_type columns[3];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0), col_type(0, s), col_type(0, 0) } { }
        constexpr matrix(T x0, T y0, T x1, T y1, T x2, T y2) : columns{ col_type(x0, y0), col_type(x1, y1), col_type(x2, y2) } {}
        constexpr matrix(const col_type& v0, const col_type& v1, const col_type& v2) : columns{ col_type(v0), col_type(v1), col_type(v2) } {}
        template<typename X0, typename Y0, typename X1, typename Y1, typename X2, typename Y2> constexpr matrix(X0 x0, Y0 y0, X1 x1, Y1 y1, X2 x2, Y2 y2) : columns{ col_type(x0, y0), col_type(x1, y1), col_type(x2, y2) } {}
        template<typename V0, typename V1, typename V2> constexpr matrix(const vector<V0,2>& v0, const vector<V1,2>& v1, const vector<V2,2>& v2) : columns{ col_type(v0), col_type(v1), col_type(v2) } {}
        template<typename U> constexpr explicit matrix(const matrix<U,3,2>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }    
        template<typename U> constexpr matrix& operator=(const matrix<U,3,2>& m) { columns[0] = m[0]; columns[1] = m[1]; columns[2] = m[2]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; columns[2] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; columns[2] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; columns[2] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; columns[2] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,3,2>& m) { columns[0] += m[0]; columns[1] += m[1]; columns[2] += m[2]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,3,2>& m) { columns[0] -= m[0]; columns[1] -= m[1]; columns[2] -= m[2]; return *this; }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; ++columns[2]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; --columns[2]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };
    
    template<typename T> constexpr typename matrix<T,3,2>::col_type operator*(const matrix<T,3,2>& m, const typename matrix<T,3,2>::row_type& v)
    {
        return typename matrix<T,3,2>::col_type(
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z, 
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z);
    }

    template<typename T> constexpr typename matrix<T,3,2>::row_type operator*(const typename matrix<T,3,2>::col_type& v, const matrix<T,3,2>& m)
    {
        return typename matrix<T,3,2>::row_type(
            v.x * m[0][0] + v.y * m[0][1], 
            v.x * m[1][0] + v.y * m[1][1], 
            v.x * m[2][0] + v.y * m[2][1]);
    }

    template<typename T> constexpr matrix<T,2,2> operator*(const matrix<T,3,2>& m1, const matrix<T,2,3>& m2)
    {
        return matrix<T,2,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2]);
    }

    template<typename T> constexpr matrix<T,3,2> operator*(const matrix<T,3,2>& m1, const matrix<T,3,3>& m2)
    {
        return matrix<T,3,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2]);
    }

    template<typename T> constexpr matrix<T,4,2> operator*(const matrix<T,3,2>& m1, const matrix<T,4,3>& m2)
    {
        return matrix<T,4,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2]);
    }

    template<typename T> constexpr matrix<T,3,2> operator+(const matrix<T,3,2>& m) { return m;}
    template<typename T> constexpr matrix<T,3,2> operator-(const matrix<T,3,2>& m) { return matrix<T,3,2>(-m[0], -m[1], -m[2]); }
    template<typename T> constexpr matrix<T,3,2> operator+(const matrix<T,3,2>& m, T s) { return matrix<T,3,2>(m[0] + s, m[1] + s, m[2] + s); }
    template<typename T> constexpr matrix<T,3,2> operator-(const matrix<T,3,2>& m, T s) { return matrix<T,3,2>(m[0] - s, m[1] - s, m[2] - s); }
    template<typename T> constexpr matrix<T,3,2> operator*(const matrix<T,3,2>& m, T s) { return matrix<T,3,2>(m[0] * s, m[1] * s, m[2] * s); }
    template<typename T> constexpr matrix<T,3,2> operator/(const matrix<T,3,2>& m, T s) { return matrix<T,3,2>(m[0] / s, m[1] / s, m[2] / s); }
    template<typename T> constexpr matrix<T,3,2> operator*(T s, const matrix<T,3,2>& m) { return matrix<T,3,2>(m[0] * s, m[1] * s, m[2] * s); }
    template<typename T> constexpr matrix<T,3,2> operator/(T s, const matrix<T,3,2>& m) { return matrix<T,3,2>(s / m[0], s / m[1], s / m[2]); }
    template<typename T> constexpr matrix<T,3,2> operator+(const matrix<T,3,2>& m1, const matrix<T,3,2>& m2) { return matrix<T,3,2>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2]); }
    template<typename T> constexpr matrix<T,3,2> operator-(const matrix<T,3,2>& m1, const matrix<T,3,2>& m2) { return matrix<T,3,2>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2]); }
    template<typename T> constexpr bool operator==(const matrix<T,3,2>& m1, const matrix<T,3,2>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]); }
    template<typename T> constexpr bool operator!=(const matrix<T,3,2>& m1, const matrix<T,3,2>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]); }


    template<typename T>
    struct matrix<T,3,3>
    {
        typedef vector<T,3> col_type;
        typedef vector<T,3> row_type;
        col_type columns[3];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0, 0), col_type(0, s, 0), col_type(0, 0, s) } {}
        constexpr matrix(T x0, T y0, T z0, T x1, T y1, T z1, T x2, T y2, T z2) : columns{ col_type(x0, y0, z0), col_type(x1, y1, z1), col_type(x2, y2, z2) } {}
        constexpr matrix(const col_type& v0, const col_type& v1, const col_type& v2) : columns{ col_type(v0), col_type(v1), col_type(v2) } {}
        template<typename X0, typename Y0, typename Z0, typename X1, typename Y1, typename Z1, typename X2, typename Y2, typename Z2> constexpr matrix(X0 x0, Y0 y0, Z0 z0, X1 x1, Y1 y1, Z1 z1, X2 x2, Y2 y2, Z2 z2) : columns{ col_type(x0, y0, z0), col_type(x1, y1, z1), col_type(x2, y2, z2) } {}
        template<typename V0, typename V1, typename V2> constexpr matrix(const vector<V0,3>& v0, const vector<V1,3>& v1, const vector<V2,3>& v2) : columns{ col_type(v0), col_type(v1), col_type(v2) } {}
        template<typename U> constexpr explicit matrix(const matrix<U,3,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 1) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 1) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,3,3>& m) { columns[0] = m[0]; columns[1] = m[1]; columns[2] = m[2]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; columns[2] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; columns[2] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; columns[2] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; columns[2] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,3,3>& m) { columns[0] += m[0]; columns[1] += m[1]; columns[2] += m[2]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,3,3>& m) { columns[0] -= m[0]; columns[1] -= m[1]; columns[2] -= m[2]; return *this; }
        template<typename U> constexpr matrix& operator*=(const matrix<U,3,3>& m) { return (*this = *this * m); }
        template<typename U> constexpr matrix& operator/=(const matrix<U,3,3>& m) { return *this *= inverse(m); }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; ++columns[2]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; --columns[2]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };

    template<typename T> constexpr typename matrix<T,3,3>::col_type operator*(const matrix<T,3,3>& m, const typename matrix<T,3,3>::row_type& v)
    {
        return typename matrix<T,3,3>::col_type(
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z, 
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z, 
            m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z);
    }

    template<typename T> constexpr typename matrix<T,3,3>::row_type operator*(const typename matrix<T,3,3>::col_type& v, const matrix<T,3,3>& m)
    {
        return typename matrix<T,3,3>::row_type(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, 
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z, 
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
    }

    template<typename T> constexpr matrix<T,3,3> operator*(const matrix<T,3,3>& m1, const matrix<T,3,3>& m2)
    {
        return matrix<T,3,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2]);
    }

    template<typename T> constexpr matrix<T,2,3> operator*(const matrix<T,3,3>& m1, const matrix<T,2,3>& m2)
    {
        return matrix<T,2,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2]);
    }

    template<typename T> constexpr matrix<T,4,3> operator*(const matrix<T,3,3>& m1, const matrix<T,4,3>& m2)
    {
        return matrix<T,4,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2],
            m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1] + m1[2][2] * m2[3][2]);
    }

    template<typename T> constexpr matrix<T,3,3> operator+(const matrix<T,3,3>& m) { return m; }
    template<typename T> constexpr matrix<T,3,3> operator-(const matrix<T,3,3>& m) { return matrix<T,3,3>(-m[0], -m[1], -m[2]); }
    template<typename T> constexpr matrix<T,3,3> operator+(const matrix<T,3,3>& m, T s) { return matrix<T,3,3>(m[0] + s, m[1] + s, m[2] + s); }
    template<typename T> constexpr matrix<T,3,3> operator-(const matrix<T,3,3>& m, T s) { return matrix<T,3,3>(m[0] - s, m[1] - s, m[2] - s); }
    template<typename T> constexpr matrix<T,3,3> operator*(const matrix<T,3,3>& m, T s) { return matrix<T,3,3>(m[0] * s, m[1] * s, m[2] * s); }
    template<typename T> constexpr matrix<T,3,3> operator/(const matrix<T,3,3>& m, T s) { return matrix<T,3,3>(m[0] / s, m[1] / s, m[2] / s); }
    template<typename T> constexpr matrix<T,3,3> operator+(T s, const matrix<T,3,3>& m) { return matrix<T,3,3>(m[0] + s, m[1] + s, m[2] + s); }
    template<typename T> constexpr matrix<T,3,3> operator-(T s, const matrix<T,3,3>& m) { return matrix<T,3,3>(s - m[0], s - m[1], s - m[2]); }
    template<typename T> constexpr matrix<T,3,3> operator*(T s, const matrix<T,3,3>& m) { return matrix<T,3,3>(m[0] * s, m[1] * s, m[2] * s); }
    template<typename T> constexpr matrix<T,3,3> operator/(T s, const matrix<T,3,3>& m) { return matrix<T,3,3>(s / m[0], s / m[1], s / m[2]); }
    template<typename T> constexpr matrix<T,3,3> operator+(const matrix<T,3,3>& m1, const matrix<T,3,3>& m2) { return matrix<T,3,3>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2]); }
    template<typename T> constexpr matrix<T,3,3> operator-(const matrix<T,3,3>& m1, const matrix<T,3,3>& m2) { return matrix<T,3,3>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2]); }
    template<typename T> typename matrix<T,3,3>::col_type operator/(const matrix<T,3,3>& m, const typename matrix<T,3,3>::row_type& v) { return inverse(m) * v; }
    template<typename T> typename matrix<T,3,3>::row_type operator/(const typename matrix<T,3,3>::col_type& v, const matrix<T,3,3>& m) { return v * inverse(m); }
    template<typename T> constexpr matrix<T,3,3> operator/(const matrix<T,3,3>& m1, const matrix<T,3,3>& m2) { matrix<T,3,3> m1_copy(m1); return m1_copy /= m2; }
    template<typename T> constexpr bool operator==(const matrix<T,3,3>& m1, const matrix<T,3,3>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]); }
    template<typename T> constexpr bool operator!=(const matrix<T,3,3>& m1, const matrix<T,3,3>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]); }


    template<typename T>
    struct matrix<T,3,4>
    {
        typedef vector<T,4> col_type;
        typedef vector<T,3> row_type;
        col_type columns[3];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0, 0, 0), col_type(0, s, 0, 0), col_type(0, 0, s, 0) } {}
        constexpr matrix(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1, T x2, T y2, T z2, T w2) : columns{col_type(x0, y0, z0, w0), col_type(x1, y1, z1, w1), col_type(x2, y2, z2, w2) } {}
        constexpr matrix(const col_type& v0, const col_type& v1, const col_type& v2) : columns{ col_type(v0), col_type(v1), col_type(v2) } {}
        template<typename X0, typename Y0, typename Z0, typename W0, typename X1, typename Y1, typename Z1, typename W1, typename X2, typename Y2, typename Z2, typename W2>
        constexpr matrix(X0 x0, Y0 y0, Z0 z0, W0 w0, X1 x1, Y1 y1, Z1 z1, W1 w1, X2 x2, Y2 y2, Z2 z2, W2 w2) : columns{col_type(x0, y0, z0, w0), col_type(x1, y1, z1, w1), col_type(x2, y2, z2, w2) } {}
        template<typename V0, typename V1, typename V2> constexpr matrix(const vector<V0,4>& v0, const vector<V1,4>& v1, const vector<V2,4>& v2) : columns{ col_type(v0), col_type(v1), col_type(v2) } {}
        template<typename U> constexpr explicit matrix(const matrix<U,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0), col_type(0, 0, 1, 0) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 0) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(0, 0, 1, 0) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0), col_type(m[2], 1, 0) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0, 0, 1, 0) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0), col_type(m[2], 1, 0) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 0) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,3,4>& m) { columns[0] = m[0]; columns[1] = m[1]; columns[2] = m[2]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; columns[2] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; columns[2] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; columns[2] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; columns[2] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,3,4>& m) { columns[0] += m[0]; columns[1] += m[1]; columns[2] += m[2]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,3,4>& m) { columns[0] -= m[0]; columns[1] -= m[1]; columns[2] -= m[2]; return *this; }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; ++columns[2]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; --columns[2]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };
    
    template<typename T> constexpr typename matrix<T,3,4>::col_type operator*(const matrix<T,3,4>& m, const typename matrix<T,3,4>::row_type& v)
    {
        return typename matrix<T,3,4>::col_type(
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z, 
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z, 
            m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z, 
            m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z);
    }

    template<typename T> constexpr typename matrix<T,3,4>::row_type operator*(const typename matrix<T,3,4>::col_type& v, const matrix<T,3,4>& m)
    {
        return typename matrix<T,3,4>::row_type(
            v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + v.w * m[0][3], 
            v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + v.w * m[1][3], 
            v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + v.w * m[2][3]);
    }

    template<typename T> constexpr matrix<T,4,4> operator*(const matrix<T,3,4>& m1, const matrix<T,4,3>& m2)
    {
        return matrix<T,4,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1] + m1[2][3] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1] + m1[2][3] * m2[1][2],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2],
            m1[0][3] * m2[2][0] + m1[1][3] * m2[2][1] + m1[2][3] * m2[2][2],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2],
            m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1] + m1[2][2] * m2[3][2],
            m1[0][3] * m2[3][0] + m1[1][3] * m2[3][1] + m1[2][3] * m2[3][2]);
    }

    template<typename T> constexpr matrix<T,2,4> operator*(const matrix<T,3,4>& m1, const matrix<T,2,3>& m2)
    {
        return matrix<T,2,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1] + m1[2][3] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1] + m1[2][3] * m2[1][2]);
    }

    template<typename T> constexpr matrix<T,3,4> operator*(const matrix<T,3,4>& m1, const matrix<T,3,3>& m2)
    {
        return matrix<T,3,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1] + m1[2][3] * m2[0][2],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1] + m1[2][3] * m2[1][2],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2],
            m1[0][3] * m2[2][0] + m1[1][3] * m2[2][1] + m1[2][3] * m2[2][2]);
    }

    template<typename T> constexpr matrix<T,3,4> operator+(const matrix<T,3,4>& m) { return m; }
    template<typename T> constexpr matrix<T,3,4> operator-(const matrix<T,3,4>& m) { return matrix<T,3,4>(-m[0], -m[1], -m[2]); }
    template<typename T> constexpr matrix<T,3,4> operator+(const matrix<T,3,4>& m, T s) { return matrix<T,3,4>(m[0] + s, m[1] + s, m[2] + s); }
    template<typename T> constexpr matrix<T,3,4> operator-(const matrix<T,3,4>& m, T s) { return matrix<T,3,4>(m[0] - s, m[1] - s, m[2] - s); }
    template<typename T> constexpr matrix<T,3,4> operator*(const matrix<T,3,4>& m, T s) { return matrix<T,3,4>(m[0] * s, m[1] * s, m[2] * s); }
    template<typename T> constexpr matrix<T,3,4> operator/(const matrix<T,3,4>& m, T s) { return matrix<T,3,4>(m[0] / s, m[1] / s, m[2] / s); }
    template<typename T> constexpr matrix<T,3,4> operator*(T s, const matrix<T,3,4>& m) { return matrix<T,3,4>(m[0] * s, m[1] * s, m[2] * s); }
    template<typename T> constexpr matrix<T,3,4> operator/(T s, const matrix<T,3,4>& m) { return matrix<T,3,4>(s / m[0], s / m[1], s / m[2]); }
    template<typename T> constexpr matrix<T,3,4> operator+(const matrix<T,3,4>& m1, const matrix<T,3,4>& m2) { return matrix<T,3,4>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2]); }
    template<typename T> constexpr matrix<T,3,4> operator-(const matrix<T,3,4>& m1, const matrix<T,3,4>& m2) { return matrix<T,3,4>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2]); }
    template<typename T> constexpr bool operator==(const matrix<T,3,4>& m1, const matrix<T,3,4>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]); }
    template<typename T> constexpr bool operator!=(const matrix<T,3,4>& m1, const matrix<T,3,4>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]); }


    template<typename T>
    struct matrix<T,4,2>
    {
        typedef vector<T,2> col_type;
        typedef vector<T,4> row_type;
        col_type columns[4];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0), col_type(0, s), col_type(0, 0), col_type(0, 0) } {}
        constexpr matrix(T x0, T y0, T x1, T y1, T x2, T y2, T x3, T y3) : columns{ col_type(x0, y0), col_type(x1, y1), col_type(x2, y2), col_type(x3, y3) } {}
        constexpr matrix(const col_type& v0, const col_type& v1, const col_type& v2, const col_type& v3) : columns{ col_type(v0), col_type(v1), col_type(v2), col_type(v3) } {}
        template<typename X0, typename Y0, typename X1, typename Y1, typename X2, typename Y2, typename X3, typename Y3>
        constexpr matrix(X0 x0, Y0 y0, X1 x1, Y1 y1, X2 x2, Y2 y2, X3 x3, Y3 y3) : columns{ col_type(x0, y0), col_type(x1, y1), col_type(x2, y2), col_type(x3, y3) } {}
        template<typename V0, typename V1, typename V2, typename V3>
        constexpr matrix(const vector<V0,2>& v0, const vector<V1,2>& v1, const vector<V2,2>& v2, const vector<V3,2>& v3) : columns{ col_type(v0), col_type(v1), col_type(v2), col_type(v3) } {}
        template<typename U> 
        constexpr explicit matrix(const matrix<U,4,2>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(m[3]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(m[3]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(m[3]) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(0) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,4,2>& m) { columns[0] = m[0]; columns[1] = m[1]; columns[2] = m[2]; columns[3] = m[3]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; columns[2] += s; columns[3] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; columns[2] -= s; columns[3] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; columns[2] *= s; columns[3] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; columns[2] /= s; columns[3] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,4,2>& m) { columns[0] += m[0]; columns[1] += m[1]; columns[2] += m[2]; columns[3] += m[3]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,4,2>& m) { columns[0] -= m[0]; columns[1] -= m[1]; columns[2] -= m[2]; columns[3] -= m[3]; return *this; }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; ++columns[2]; ++columns[3]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; --columns[2]; --columns[3]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };
    
    template<typename T> constexpr typename matrix<T,4,2>::col_type operator*(const matrix<T,4,2>& m, const typename matrix<T,4,2>::row_type& v)
    {
        return typename matrix<T,4,2>::col_type(
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w, 
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w);
    }

    template<typename T> constexpr typename matrix<T,4,2>::row_type operator*(const typename matrix<T,4,2>::col_type& v, const matrix<T,4,2>& m)
    {
        return typename matrix<T,4,2>::row_type(
            v.x * m[0][0] + v.y * m[0][1], v.x * m[1][0] + v.y * m[1][1], 
            v.x * m[2][0] + v.y * m[2][1], v.x * m[3][0] + v.y * m[3][1]);
    }

    template<typename T> constexpr matrix<T,2,2> operator*(const matrix<T,4,2>& m1, const matrix<T,2,4>& m2)
    {
        return matrix<T,2,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3]);
    }

    template<typename T> constexpr matrix<T,3,2> operator*(const matrix<T,4,2>& m1, const matrix<T,3,4>& m2)
    {
        return matrix<T,3,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2] + m1[3][0] * m2[2][3],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2] + m1[3][1] * m2[2][3]);
    }

    template<typename T> constexpr matrix<T,4,2> operator*(const matrix<T,4,2>& m1, const matrix<T,4,4>& m2)
    {
        return matrix<T,4,2>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2] + m1[3][0] * m2[2][3],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2] + m1[3][1] * m2[2][3],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2] + m1[3][0] * m2[3][3],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2] + m1[3][1] * m2[3][3]);
    }

    template<typename T> constexpr matrix<T,4,2> operator+(const matrix<T,4,2>& m) { return m; }
    template<typename T> constexpr matrix<T,4,2> operator-(const matrix<T,4,2>& m) { return matrix<T,4,2>(-m[0], -m[1], -m[2], -m[3]); }
    template<typename T> constexpr matrix<T,4,2> operator+(const matrix<T,4,2>& m, T s) { return matrix<T,4,2>(m[0] + s, m[1] + s, m[2] + s, m[3] + s); }
    template<typename T> constexpr matrix<T,4,2> operator-(const matrix<T,4,2>& m, T s) { return matrix<T,4,2>(m[0] - s, m[1] - s, m[2] - s, m[3] - s); }
    template<typename T> constexpr matrix<T,4,2> operator*(const matrix<T,4,2>& m, T s) { return matrix<T,4,2>(m[0] * s, m[1] * s, m[2] * s, m[3] * s); }
    template<typename T> constexpr matrix<T,4,2> operator/(const matrix<T,4,2>& m, T s) { return matrix<T,4,2>(m[0] / s, m[1] / s, m[2] / s, m[3] / s); }
    template<typename T> constexpr matrix<T,4,2> operator*(T s, const matrix<T,4,2>& m) { return matrix<T,4,2>(m[0] * s, m[1] * s, m[2] * s, m[3] * s); }
    template<typename T> constexpr matrix<T,4,2> operator/(T s, const matrix<T,4,2>& m) { return matrix<T,4,2>(s / m[0], s / m[1], s / m[2], s / m[3]); }
    template<typename T> constexpr matrix<T,4,2> operator+(const matrix<T,4,2>& m1, const matrix<T,4,2>& m2) { return matrix<T,4,2>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]); }
    template<typename T> constexpr matrix<T,4,2> operator-(const matrix<T,4,2>& m1, const matrix<T,4,2>& m2) { return matrix<T,4,2>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]); }
    template<typename T> constexpr bool operator==(const matrix<T,4,2>& m1, const matrix<T,4,2>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]) && (m1[3] == m2[3]); }
    template<typename T> constexpr bool operator!=(const matrix<T,4,2>& m1, const matrix<T,4,2>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]) || (m1[3] != m2[3]); }


    template<typename T>
    struct matrix<T,4,3>
    {
        typedef vector<T,3> col_type;
        typedef vector<T,4> row_type;
        col_type columns[4];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0, 0), col_type(0, s, 0), col_type(0, 0, s), col_type(0, 0, 0) } {}
        constexpr matrix(T x0, T y0, T z0, T x1, T y1, T z1, T x2, T y2, T z2, T x3, T y3, T z3) : columns{ col_type(x0, y0, z0), col_type(x1, y1, z1), col_type(x2, y2, z2), col_type(x3, y3, z3) } {}
        constexpr matrix(const col_type& v0, const col_type& v1, const col_type& v2, const col_type& v3) : columns{ col_type(v0), col_type(v1), col_type(v2), col_type(v3) } {}
        template<typename X0, typename Y0, typename Z0, typename X1, typename Y1, typename Z1, typename X2, typename Y2, typename Z2, typename X3, typename Y3, typename Z3>
        constexpr matrix(X0 x0, Y0 y0, Z0 z0, X1 x1, Y1 y1, Z1 z1, X2 x2, Y2 y2, Z2 z2, X3 x3, Y3 y3, Z3 z3) : columns{ col_type(x0, y0, z0), col_type(x1, y1, z1), col_type(x2, y2, z2), col_type(x3, y3, z3) } {}
        template<typename V0, typename V1, typename V2, typename V3>
        constexpr matrix(const vector<V0,3>& v0, const vector<V1,3>& v1, const vector<V2,3>& v2, const vector<V3,3>& v3) : columns{ col_type(v0), col_type(v1), col_type(v2), col_type(v3) } {}
        template<typename U>
        constexpr explicit matrix(const matrix<U,4,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(m[3]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(0, 0, 1), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(m[3]) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0, 0, 1), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 1), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0, 0, 1), col_type(0) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 1), col_type(m[3], 0) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(0) } {}

        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,4,3>& m) { columns[0] = m[0]; columns[1] = m[1]; columns[2] = m[2]; columns[3] = m[3]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; columns[2] += s; columns[3] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; columns[2] -= s; columns[3] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; columns[2] *= s; columns[3] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; columns[2] /= s; columns[3] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,4,3>& m) { columns[0] += m[0]; columns[1] += m[1]; columns[2] += m[2]; columns[3] += m[3]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,4,3>& m) { columns[0] -= m[0]; columns[1] -= m[1]; columns[2] -= m[2]; columns[3] -= m[3]; return *this; }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; ++columns[2]; ++columns[3]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; --columns[2]; --columns[3]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };

    template<typename T> constexpr typename matrix<T,4,3>::col_type operator*(const matrix<T,4,3>& m, const typename matrix<T,4,3>::row_type& v)
    {
        return typename matrix<T,4,3>::col_type(
            m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w, 
            m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w, 
            m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w);
    }

    template<typename T> constexpr typename matrix<T,4,3>::row_type operator*(const typename matrix<T,4,3>::col_type& v, const matrix<T,4,3>& m)
    {
        return typename matrix<T,4,3>::row_type(
            v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2], 
            v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2], 
            v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2], 
            v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2]);
    }

    template<typename T> constexpr matrix<T,2,3> operator*(const matrix<T,4,3>& m1, const matrix<T,2,4>& m2)
    {
        return matrix<T,2,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2] + m1[3][2] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2] + m1[3][2] * m2[1][3]);
    }

    template<typename T> constexpr matrix<T,3,3> operator*(const matrix<T,4,3>& m1, const matrix<T,3,4>& m2)
    {
        return matrix<T,3,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2] + m1[3][2] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2] + m1[3][2] * m2[1][3],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2] + m1[3][0] * m2[2][3],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2] + m1[3][1] * m2[2][3],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2] + m1[3][2] * m2[2][3]);
    }

    template<typename T> constexpr matrix<T,4,3> operator*(const matrix<T,4,3>& m1, const matrix<T,4,4>& m2)
    {
        return matrix<T,4,3>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2] + m1[3][2] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2] + m1[3][2] * m2[1][3],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2] + m1[3][0] * m2[2][3],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2] + m1[3][1] * m2[2][3],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2] + m1[3][2] * m2[2][3],
            m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2] + m1[3][0] * m2[3][3],
            m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2] + m1[3][1] * m2[3][3],
            m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1] + m1[2][2] * m2[3][2] + m1[3][2] * m2[3][3]);
    }
    
    template<typename T> constexpr matrix<T,4,3> operator+(const matrix<T,4,3>& m) { return m; }
    template<typename T> constexpr matrix<T,4,3> operator-(const matrix<T,4,3>& m) { return matrix<T,4,3>(-m[0], -m[1], -m[2], -m[3]); }
    template<typename T> constexpr matrix<T,4,3> operator+(const matrix<T,4,3>& m, T s) { return matrix<T,4,3>(m[0] + s, m[1] + s, m[2] + s, m[3] + s); }
    template<typename T> constexpr matrix<T,4,3> operator-(const matrix<T,4,3>& m, T s) { return matrix<T,4,3>(m[0] - s, m[1] - s, m[2] - s, m[3] - s); }
    template<typename T> constexpr matrix<T,4,3> operator*(const matrix<T,4,3>& m, T s) { return matrix<T,4,3>(m[0] * s, m[1] * s, m[2] * s, m[3] * s); }
    template<typename T> constexpr matrix<T,4,3> operator/(const matrix<T,4,3>& m, T s) { return matrix<T,4,3>(m[0] / s, m[1] / s, m[2] / s, m[3] / s); }
    template<typename T> constexpr matrix<T,4,3> operator*(T s, const matrix<T,4,3>& m) { return matrix<T,4,3>(m[0] * s, m[1] * s, m[2] * s, m[3] * s); }
    template<typename T> constexpr matrix<T,4,3> operator/(T s, const matrix<T,4,3>& m) { return matrix<T,4,3>(s / m[0], s / m[1], s / m[2], s / m[3]); }
    template<typename T> constexpr matrix<T,4,3> operator+(const matrix<T,4,3>& m1, const matrix<T,4,3>& m2) { return matrix<T,4,3>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]); }
    template<typename T> constexpr matrix<T,4,3> operator-(const matrix<T,4,3>& m1, const matrix<T,4,3>& m2) { return matrix<T,4,3>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]); }
    template<typename T> constexpr bool operator==(const matrix<T,4,3>& m1, const matrix<T,4,3>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]) && (m1[3] == m2[3]); }
    template<typename T> constexpr bool operator!=(const matrix<T,4,3>& m1, const matrix<T,4,3>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]) || (m1[3] != m2[3]); }


    template<typename T>
    struct matrix<T,4,4>
    {
        typedef vector<T,4> col_type;
        typedef vector<T,4> row_type;
        col_type columns[4];

        constexpr matrix() = default;
        constexpr matrix(const matrix& m) = default;
        constexpr explicit matrix(T s) : columns{ col_type(s, 0, 0, 0), col_type(0, s, 0, 0), col_type(0, 0, s, 0), col_type(0, 0, 0, s) } {}
        constexpr matrix(T x0, T y0, T z0, T w0, T x1, T y1, T z1, T w1, T x2, T y2, T z2, T w2, T x3, T y3, T z3, T w3) : columns{col_type(x0, y0, z0, w0), col_type(x1, y1, z1, w1), col_type(x2, y2, z2, w2), col_type(x3, y3, z3, w3) } {}
        constexpr matrix(const col_type& v0, const col_type& v1, const col_type& v2, const col_type& v3) : columns{ col_type(v0), col_type(v1), col_type(v2), col_type(v3) } {}
        template<typename X0, typename Y0, typename Z0, typename W0, typename X1, typename Y1, typename Z1, typename W1, typename X2, typename Y2, typename Z2, typename W2, typename X3, typename Y3, typename Z3, typename W3>
        constexpr matrix(X0 x0, Y0 y0, Z0 z0, W0 w0, X1 x1, Y1 y1, Z1 z1, W1 w1, X2 x2, Y2 y2, Z2 z2, W2 w2, X3 x3, Y3 y3, Z3 z3, W3 w3) : columns{ col_type(x0, y0, z0, w0), col_type(x1, y1, z1, w1), col_type(x2, y2, z2, w2), col_type(x3, y3, z3, w3) } {}
        template<typename V0, typename V1, typename V2, typename V3>
        constexpr matrix(const vector<V0,4>& v0, const vector<V1,4>& v1, const vector<V2,4>& v2, const vector<V3,4>& v3) : columns{ col_type(v0), col_type(v1), col_type(v2), col_type(v3) } {}
        template<typename U>
        constexpr explicit matrix(const matrix<U,4,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(m[3]) } {}
        constexpr explicit matrix(const matrix<T,2,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0), col_type(0, 0, 1, 0), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,3,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 0), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,2,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(0, 0, 1, 0), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,3,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0), col_type(m[2], 1, 0), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,2,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(0, 0, 1, 0), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,4,2>& m) : columns{ col_type(m[0], 0, 0), col_type(m[1], 0, 0), col_type(0, 0, 1, 0), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,3,4>& m) : columns{ col_type(m[0]), col_type(m[1]), col_type(m[2]), col_type(0, 0, 0, 1) } {}
        constexpr explicit matrix(const matrix<T,4,3>& m) : columns{ col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 0), col_type(m[3], 1) } {}
        
        constexpr col_type& operator[](int i) { return columns[i]; }
        constexpr const col_type& operator[](int i) const { return columns[i]; }
        template<typename U> constexpr matrix& operator=(const matrix<U,4,4>& m) { columns[0] = m[0]; columns[1] = m[1]; columns[2] = m[2]; columns[3] = m[3]; return *this; }
        template<typename U> constexpr matrix& operator+=(U s) { columns[0] += s; columns[1] += s; columns[2] += s; columns[3] += s; return *this; }
        template<typename U> constexpr matrix& operator-=(U s) { columns[0] -= s; columns[1] -= s; columns[2] -= s; columns[3] -= s; return *this; }
        template<typename U> constexpr matrix& operator*=(U s) { columns[0] *= s; columns[1] *= s; columns[2] *= s; columns[3] *= s; return *this; }
        template<typename U> constexpr matrix& operator/=(U s) { columns[0] /= s; columns[1] /= s; columns[2] /= s; columns[3] /= s; return *this; }
        template<typename U> constexpr matrix& operator+=(const matrix<U,4,4>& m) { columns[0] += m[0]; columns[1] += m[1]; columns[2] += m[2]; columns[3] += m[3]; return *this; }
        template<typename U> constexpr matrix& operator-=(const matrix<U,4,4>& m) { columns[0] -= m[0]; columns[1] -= m[1]; columns[2] -= m[2]; columns[3] -= m[3]; return *this; }
        template<typename U> constexpr matrix& operator*=(const matrix<U,4,4>& m) { return (*this = *this * m); }
        template<typename U> constexpr matrix& operator/=(const matrix<U,4,4>& m) { return *this *= inverse(m); }
        constexpr matrix& operator++() { ++columns[0]; ++columns[1]; ++columns[2]; ++columns[3]; return *this; }
        constexpr matrix& operator--() { --columns[0]; --columns[1]; --columns[2]; --columns[3]; return *this; }
        constexpr matrix operator++(int) { matrix result(*this); ++* this; return result; }
        constexpr matrix operator--(int) { matrix result(*this); --* this; return result; }
    };

    template<typename T> constexpr typename matrix<T,4,4>::col_type operator*(const matrix<T,4,4>& m, const typename matrix<T,4,4>::row_type& v)
    {
        typename matrix<T,4,4>::col_type const Mov0(v[0]);
        typename matrix<T,4,4>::col_type const Mov1(v[1]);
        typename matrix<T,4,4>::col_type const Mul0 = m[0] * Mov0;
        typename matrix<T,4,4>::col_type const Mul1 = m[1] * Mov1;
        typename matrix<T,4,4>::col_type const Add0 = Mul0 + Mul1;
        typename matrix<T,4,4>::col_type const Mov2(v[2]);
        typename matrix<T,4,4>::col_type const Mov3(v[3]);
        typename matrix<T,4,4>::col_type const Mul2 = m[2] * Mov2;
        typename matrix<T,4,4>::col_type const Mul3 = m[3] * Mov3;
        typename matrix<T,4,4>::col_type const Add1 = Mul2 + Mul3;
        typename matrix<T,4,4>::col_type const Add2 = Add0 + Add1;
        return Add2;
    }

    template<typename T> constexpr typename matrix<T,4,4>::row_type operator*(const typename matrix<T,4,4>::col_type& v, const matrix<T,4,4>& m)
    {
        return typename matrix<T,4,4>::row_type(
            m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2] + m[0][3] * v[3],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2] + m[1][3] * v[3],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2] + m[2][3] * v[3],
            m[3][0] * v[0] + m[3][1] * v[1] + m[3][2] * v[2] + m[3][3] * v[3]);
    }

    template<typename T> constexpr matrix<T,2,4> operator*(const matrix<T,4,4>& m1, const matrix<T,2,4>& m2)
    {
        return matrix<T,2,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2] + m1[3][2] * m2[0][3],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1] + m1[2][3] * m2[0][2] + m1[3][3] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2] + m1[3][2] * m2[1][3],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1] + m1[2][3] * m2[1][2] + m1[3][3] * m2[1][3]);
    }

    template<typename T> constexpr matrix<T,3,4> operator*(const matrix<T,4,4>& m1, const matrix<T,3,4>& m2)
    {
        return matrix<T,3,4>(
            m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2] + m1[3][0] * m2[0][3],
            m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2] + m1[3][1] * m2[0][3],
            m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2] + m1[3][2] * m2[0][3],
            m1[0][3] * m2[0][0] + m1[1][3] * m2[0][1] + m1[2][3] * m2[0][2] + m1[3][3] * m2[0][3],
            m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2] + m1[3][0] * m2[1][3],
            m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2] + m1[3][1] * m2[1][3],
            m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2] + m1[3][2] * m2[1][3],
            m1[0][3] * m2[1][0] + m1[1][3] * m2[1][1] + m1[2][3] * m2[1][2] + m1[3][3] * m2[1][3],
            m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2] + m1[3][0] * m2[2][3],
            m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2] + m1[3][1] * m2[2][3],
            m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2] + m1[3][2] * m2[2][3],
            m1[0][3] * m2[2][0] + m1[1][3] * m2[2][1] + m1[2][3] * m2[2][2] + m1[3][3] * m2[2][3]);
    }

    template<typename T> constexpr matrix<T,4,4> operator*(const matrix<T,4,4>& m1, const matrix<T,4,4>& m2)
    {
        return matrix<T,4,4>(
            m1[0] * m2[0][0] + m1[1] * m2[0][1] + m1[2] * m2[0][2] + m1[3] * m2[0][3],
            m1[0] * m2[1][0] + m1[1] * m2[1][1] + m1[2] * m2[1][2] + m1[3] * m2[1][3],
            m1[0] * m2[2][0] + m1[1] * m2[2][1] + m1[2] * m2[2][2] + m1[3] * m2[2][3],
            m1[0] * m2[3][0] + m1[1] * m2[3][1] + m1[2] * m2[3][2] + m1[3] * m2[3][3]);
    }

    template<typename T> constexpr matrix<T,4,4> operator+(const matrix<T,4,4>& m) { return m; }
    template<typename T> constexpr matrix<T,4,4> operator-(const matrix<T,4,4>& m) { return matrix<T,4,4>(-m[0], -m[1], -m[2], -m[3]); }
    template<typename T> constexpr matrix<T,4,4> operator+(const matrix<T,4,4>& m, T s) { return matrix<T,4,4>(m[0] + s, m[1] + s, m[2] + s, m[3] + s); }
    template<typename T> constexpr matrix<T,4,4> operator-(const matrix<T,4,4>& m, T s) { return matrix<T,4,4>(m[0] - s, m[1] - s, m[2] - s, m[3] - s); }
    template<typename T> constexpr matrix<T,4,4> operator*(const matrix<T,4,4>& m, T s) { return matrix<T,4,4>(m[0] * s, m[1] * s, m[2] * s, m[3] * s); }
    template<typename T> constexpr matrix<T,4,4> operator/(const matrix<T,4,4>& m, T s) { return matrix<T,4,4>(m[0] / s, m[1] / s, m[2] / s, m[3] / s); }
    template<typename T> constexpr matrix<T,4,4> operator+(T s, const matrix<T,4,4>& m) { return matrix<T,4,4>(m[0] + s, m[1] + s, m[2] + s, m[3] + s); }
    template<typename T> constexpr matrix<T,4,4> operator-(T s, const matrix<T,4,4>& m) { return matrix<T,4,4>(s - m[0], s - m[1], s - m[2], s - m[3]); }
    template<typename T> constexpr matrix<T,4,4> operator*(T s, const matrix<T,4,4>& m) { return matrix<T,4,4>(m[0] * s, m[1] * s, m[2] * s, m[3] * s); }
    template<typename T> constexpr matrix<T,4,4> operator/(T s, const matrix<T,4,4>& m) { return matrix<T,4,4>(s / m[0], s / m[1], s / m[2], s / m[3]); }
    template<typename T> constexpr matrix<T,4,4> operator+(const matrix<T,4,4>& m1, const matrix<T,4,4>& m2) { return matrix<T,4,4>(m1[0] + m2[0], m1[1] + m2[1], m1[2] + m2[2], m1[3] + m2[3]); }
    template<typename T> constexpr matrix<T,4,4> operator-(const matrix<T,4,4>& m1, const matrix<T,4,4>& m2) { return matrix<T,4,4>(m1[0] - m2[0], m1[1] - m2[1], m1[2] - m2[2], m1[3] - m2[3]); }
    template<typename T> typename matrix<T,4,4>::col_type operator/(const matrix<T,4,4>& m, const typename matrix<T,4,4>::row_type& v) { return inverse(m) * v; }
    template<typename T> typename matrix<T,4,4>::row_type operator/(const typename matrix<T,4,4>::col_type& v, const matrix<T,4,4>& m) { return v * inverse(m); }
    template<typename T> constexpr matrix<T,4,4> operator/(const matrix<T,4,4>& m1, const matrix<T,4,4>& m2) { matrix<T,4,4> m1_copy(m1); return m1_copy /= m2; }
    template<typename T> constexpr bool operator==(const matrix<T,4,4>& m1, const matrix<T,4,4>& m2) { return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]) && (m1[3] == m2[3]); }
    template<typename T> constexpr bool operator!=(const matrix<T,4,4>& m1, const matrix<T,4,4>& m2) { return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]) || (m1[3] != m2[3]); }  

    constexpr bool all(const vector<bool, 2>& v) { return v.x && v.y; }
    constexpr bool all(const vector<bool, 3>& v) { return v.x && v.y && v.z; }
    constexpr bool all(const vector<bool, 4>& v) { return v.x && v.y && v.z && v.w; }
    constexpr bool any(const vector<bool, 2>& v) { return v.x || v.y; }
    constexpr bool any(const vector<bool, 3>& v) { return v.x || v.y || v.z; }
    constexpr bool any(const vector<bool, 4>& v) { return v.x || v.y || v.z || v.w; }

    template<typename T> vector<bool,2> isnan(const vector<T,2>& v) { return vector<bool,2>(isnan(v.x), isnan(v.y)); }
    template<typename T> vector<bool,3> isnan(const vector<T,3>& v) { return vector<bool,3>(isnan(v.x), isnan(v.y), isnan(v.z)); }
    template<typename T> vector<bool,4> isnan(const vector<T,4>& v) { return vector<bool,4>(isnan(v.x), isnan(v.y), isnan(v.z), isnan(v.w)); }

    template<typename T> vector<bool,2> isinf(const vector<T,2>& v) { return vector<bool,2>(isinf(v.x), isinf(v.y)); }
    template<typename T> vector<bool,3> isinf(const vector<T,3>& v) { return vector<bool,3>(isinf(v.x), isinf(v.y), isinf(v.z)); }
    template<typename T> vector<bool,4> isinf(const vector<T,4>& v) { return vector<bool,4>(isinf(v.x), isinf(v.y), isinf(v.z), isinf(v.w)); }

    template<typename T> vector<bool,2> nearEqual(const vector<T,2>& x, const vector<T,2>& y, T e) { return vector<bool,2>(nearEqual(x.x, y.x, e), nearEqual(x.y, y.y, e)); }
    template<typename T> vector<bool,3> nearEqual(const vector<T,3>& x, const vector<T,3>& y, T e) { return vector<bool,3>(nearEqual(x.x, y.x, e), nearEqual(x.y, y.y, e), nearEqual(x.z, y.z, e)); }
    template<typename T> vector<bool,4> nearEqual(const vector<T,4>& x, const vector<T,4>& y, T e) { return vector<bool,4>(nearEqual(x.x, y.x, e), nearEqual(x.y, y.y, e), nearEqual(x.z, y.z, e), nearEqual(x.w, y.w, e)); }

    template<int N> constexpr vector<float,N> asfloat(const vector<int32_t,N>& v) { union { vector<int32_t,N> in; vector<float,N> out; } u; u.in = v; return u.out; }
    template<int N> constexpr vector<float,N> asfloat(const vector<uint32_t,N>& v) { union { vector<uint32_t,N> in; vector<float,N> out; } u; u.in = v; return u.out; }
    template<int N> constexpr vector<double,N> asdouble(const vector<int64_t,N>& v) { union { vector<int64_t,N> in; vector<double,N> out; } u; u.in = v; return u.out; }
    template<int N> constexpr vector<double,N> asdouble(const vector<uint64_t,N>& v) { union { vector<uint64_t,N> in; vector<double,N> out; } u; u.in = v; return u.out; }

    template<int N> constexpr vector<int32_t,N> asint(const vector<float,N>& v) { union { vector<float,N> in; vector<int32_t,N> out; } u; u.in = v; return u.out; }
    template<int N> constexpr vector<uint32_t,N> asuint(const vector<float,N>& v) { union { vector<float,N> in; vector<uint32_t,N> out; } u; u.in = v; return u.out; }
    template<int N> constexpr vector<int64_t,N> asint(const vector<double,N>& v) { union { vector<double,N> in; vector<int64_t,N> out; } u; u.in = v; return u.out; }
    template<int N> constexpr vector<uint64_t,N> asuint(const vector<double,N>& v) { union { vector<double,N> in; vector<uint64_t,N> out; } u; u.in = v; return u.out; }

    template<typename T> uint2 bitcount(const vector<T,2>& v) { return uint2(bitcount(v.x), bitcount(v.y)); }
    template<typename T> uint3 bitcount(const vector<T,3>& v) { return uint3(bitcount(v.x), bitcount(v.y), bitcount(v.z)); }
    template<typename T> uint4 bitcount(const vector<T,4>& v) { return uint4(bitcount(v.x), bitcount(v.y), bitcount(v.z), bitcount(v.w)); }

    constexpr ushort2 f32tof16(const float2& v) { return ushort2(f32tof16(v.x), f32tof16(v.y)); }
    constexpr ushort3 f32tof16(const float3& v) { return ushort3(f32tof16(v.x), f32tof16(v.y), f32tof16(v.z)); }
    constexpr ushort4 f32tof16(const float4& v) { return ushort4(f32tof16(v.x), f32tof16(v.y), f32tof16(v.z), f32tof16(v.w)); }
    constexpr float2 f16tof32(const ushort2& v) { return float2(f16tof32(v.x), f16tof32(v.y)); }
    constexpr float3 f16tof32(const ushort3& v) { return float3(f16tof32(v.x), f16tof32(v.y), f16tof32(v.z)); }
    constexpr float4 f16tof32(const ushort4& v) { return float4(f16tof32(v.x), f16tof32(v.y), f16tof32(v.z), f16tof32(v.w)); }

    constexpr uint32_t f32tof16_pack(const float2& v) { return static_cast<uint32_t>(f32tof16(v.x)) | (static_cast<uint32_t>(f32tof16(v.y)) << 16u); }
    constexpr uint64_t f32tof16_pack(const float4& v) { return static_cast<uint64_t>(f32tof16(v.x)) | (static_cast<uint64_t>(f32tof16(v.y)) << 16ull) | (static_cast<uint64_t>(f32tof16(v.y)) << 32ull) | (static_cast<uint64_t>(f32tof16(v.y)) << 48ull); }
    constexpr float2 f16tof32_unpack(uint32_t v) { return float2(f16tof32(v & 0xFFFFu), f16tof32((v >> 16u) & 0xFFFFu)); }
    constexpr float4 f16tof32_unpack(uint64_t v) { return float4(f16tof32(v & 0xFFFFu), f16tof32((v >> 16u) & 0xFFFFu), f16tof32((v >> 32u) & 0xFFFFu), f16tof32((v >> 48u) & 0xFFFFu)); }

    constexpr sbyte2 packSnorm8(const float2& v) { return sbyte2(packSnorm8(v.x), packSnorm8(v.y)); }
    constexpr sbyte3 packSnorm8(const float3& v) { return sbyte3(packSnorm8(v.x), packSnorm8(v.y), packSnorm8(v.z)); }
    constexpr sbyte4 packSnorm8(const float4& v) { return sbyte4(packSnorm8(v.x), packSnorm8(v.y), packSnorm8(v.z), packSnorm8(v.w)); }
    constexpr short2 packSnorm16(const float2& v) { return short2(packSnorm16(v.x), packSnorm16(v.y)); }
    constexpr short3 packSnorm16(const float3& v) { return short3(packSnorm16(v.x), packSnorm16(v.y), packSnorm16(v.z)); }
    constexpr short4 packSnorm16(const float4& v) { return short4(packSnorm16(v.x), packSnorm16(v.y), packSnorm16(v.z), packSnorm16(v.w)); }

    constexpr float2 unpackSnorm8(const sbyte2& v) { return float2(unpackSnorm8(v.x), unpackSnorm8(v.y)); }
    constexpr float3 unpackSnorm8(const sbyte3& v) { return float3(unpackSnorm8(v.x), unpackSnorm8(v.y), unpackSnorm8(v.z)); }
    constexpr float4 unpackSnorm8(const sbyte4& v) { return float4(unpackSnorm8(v.x), unpackSnorm8(v.y), unpackSnorm8(v.z), unpackSnorm8(v.w)); }
    constexpr float2 unpackSnorm16(const short2& v) { return float2(unpackSnorm16(v.x), unpackSnorm16(v.y)); }
    constexpr float3 unpackSnorm16(const short3& v) { return float3(unpackSnorm16(v.x), unpackSnorm16(v.y), unpackSnorm16(v.z)); }
    constexpr float4 unpackSnorm16(const short4& v) { return float4(unpackSnorm16(v.x), unpackSnorm16(v.y), unpackSnorm16(v.z), packSnorm16(v.w)); }

    constexpr byte2 packUnorm8(const float2& v) { return byte2(packUnorm8(v.x), packUnorm8(v.y)); }
    constexpr byte3 packUnorm8(const float3& v) { return byte3(packUnorm8(v.x), packUnorm8(v.y), packUnorm8(v.z)); }
    constexpr byte4 packUnorm8(const float4& v) { return byte4(packUnorm8(v.x), packUnorm8(v.y), packUnorm8(v.z), packUnorm8(v.w)); }
    constexpr ushort2 packUnorm16(const float2& v) { return ushort2(packUnorm16(v.x), packUnorm16(v.y)); }
    constexpr ushort3 packUnorm16(const float3& v) { return ushort3(packUnorm16(v.x), packUnorm16(v.y), packUnorm16(v.z)); }
    constexpr ushort4 packUnorm16(const float4& v) { return ushort4(packUnorm16(v.x), packUnorm16(v.y), packUnorm16(v.z), packUnorm16(v.w)); }

    constexpr float2 unpackUnorm8(const byte2& v) { return float2(unpackUnorm8(v.x), unpackUnorm8(v.y)); }
    constexpr float3 unpackUnorm8(const byte3& v) { return float3(unpackUnorm8(v.x), unpackUnorm8(v.y), unpackUnorm8(v.z)); }
    constexpr float4 unpackUnorm8(const byte4& v) { return float4(unpackUnorm8(v.x), unpackUnorm8(v.y), unpackUnorm8(v.z), unpackUnorm8(v.w)); }
    constexpr float2 unpackUnorm16(const ushort2& v) { return float2(unpackUnorm16(v.x), unpackUnorm16(v.y)); }
    constexpr float3 unpackUnorm16(const ushort3& v) { return float3(unpackUnorm16(v.x), unpackUnorm16(v.y), unpackUnorm16(v.z)); }
    constexpr float4 unpackUnorm16(const ushort4& v) { return float4(unpackUnorm16(v.x), unpackUnorm16(v.y), unpackUnorm16(v.z), packUnorm16(v.w)); }

    template<typename T> vector<T,2> sin(const vector<T,2>& v) { return vector<T,2>(sin(v.x), sin(v.y)); }
    template<typename T> vector<T,3> sin(const vector<T,3>& v) { return vector<T,3>(sin(v.x), sin(v.y), sin(v.z)); }
    template<typename T> vector<T,4> sin(const vector<T,4>& v) { return vector<T,4>(sin(v.x), sin(v.y), sin(v.z), sin(v.w)); }

    template<typename T> vector<T,2> cos(const vector<T,2>& v) { return vector<T,2>(cos(v.x), cos(v.y)); }
    template<typename T> vector<T,3> cos(const vector<T,3>& v) { return vector<T,3>(cos(v.x), cos(v.y), cos(v.z)); }
    template<typename T> vector<T,4> cos(const vector<T,4>& v) { return vector<T,3>(cos(v.x), cos(v.y), cos(v.z), cos(v.w)); }

    template<typename T> vector<T,2> tan(const vector<T,2>& v) { return vector<T,2>(tan(v.x), tan(v.y)); }
    template<typename T> vector<T,3> tan(const vector<T,3>& v) { return vector<T,3>(tan(v.x), tan(v.y), tan(v.z)); }
    template<typename T> vector<T,4> tan(const vector<T,4>& v) { return vector<T,4>(tan(v.x), tan(v.y), tan(v.z), tan(v.w)); }

    template<typename T> vector<T,2> asin(const vector<T,2>& v) { return vector<T,2>(asin(v.x), asin(v.y)); }
    template<typename T> vector<T,3> asin(const vector<T,3>& v) { return vector<T,3>(asin(v.x), asin(v.y), asin(v.z)); }
    template<typename T> vector<T,4> asin(const vector<T,4>& v) { return vector<T,4>(asin(v.x), asin(v.y), asin(v.z), asin(v.w)); }

    template<typename T> vector<T,2> acos(const vector<T,2>& v) { return vector<T,2>(acos(v.x), acos(v.y)); }
    template<typename T> vector<T,3> acos(const vector<T,3>& v) { return vector<T,3>(acos(v.x), acos(v.y), acos(v.z)); }
    template<typename T> vector<T,4> acos(const vector<T,4>& v) { return vector<T,4>(acos(v.x), acos(v.y), acos(v.z), acos(v.w)); }

    template<typename T> vector<T,2> atan(const vector<T,2>& v) { return vector<T,2>(atan(v.x), atan(v.y)); }
    template<typename T> vector<T,3> atan(const vector<T,3>& v) { return vector<T,3>(atan(v.x), atan(v.y), atan(v.z)); }
    template<typename T> vector<T,4> atan(const vector<T,4>& v) { return vector<T,4>(atan(v.x), atan(v.y), atan(v.z), acos(v.w)); }

    template<typename T> vector<T,2> sinh(const vector<T,2>& v) { return vector<T,2>(sinh(v.x), sinh(v.y)); }
    template<typename T> vector<T,3> sinh(const vector<T,3>& v) { return vector<T,3>(sinh(v.x), sinh(v.y), sinh(v.z)); }
    template<typename T> vector<T,4> sinh(const vector<T,4>& v) { return vector<T,4>(sinh(v.x), sinh(v.y), sinh(v.z), sinh(v.w)); }

    template<typename T> vector<T,2> cosh(const vector<T,2>& v) { return vector<T,2>(cosh(v.x), cosh(v.y)); }
    template<typename T> vector<T,3> cosh(const vector<T,3>& v) { return vector<T,3>(cosh(v.x), cosh(v.y), cosh(v.z)); }
    template<typename T> vector<T,4> cosh(const vector<T,4>& v) { return vector<T,4>(cosh(v.x), cosh(v.y), cosh(v.z), cosh(v.w)); }
    
    template<typename T> vector<T,2> tanh(const vector<T,2>& v) { return vector<T,2>(tanh(v.x), tanh(v.y)); }
    template<typename T> vector<T,3> tanh(const vector<T,3>& v) { return vector<T,3>(tanh(v.x), tanh(v.y), tanh(v.z)); }
    template<typename T> vector<T,4> tanh(const vector<T,4>& v) { return vector<T,4>(tanh(v.x), tanh(v.y), tanh(v.z), tanh(v.w)); }

    template<typename T> vector<T,2> asinh(const vector<T,2>& v) { return vector<T,2>(asinh(v.x), asinh(v.y)); }
    template<typename T> vector<T,3> asinh(const vector<T,3>& v) { return vector<T,3>(asinh(v.x), asinh(v.y), asinh(v.z)); }
    template<typename T> vector<T,4> asinh(const vector<T,4>& v) { return vector<T,4>(asinh(v.x), asinh(v.y), asinh(v.z), asinh(v.w)); }

    template<typename T> vector<T,2> acosh(const vector<T,2>& v) { return vector<T,2>(acosh(v.x), acosh(v.y)); }
    template<typename T> vector<T,3> acosh(const vector<T,3>& v) { return vector<T,3>(acosh(v.x), acosh(v.y), acosh(v.z)); }
    template<typename T> vector<T,4> acosh(const vector<T,4>& v) { return vector<T,4>(acosh(v.x), acosh(v.y), acosh(v.z), acosh(v.w)); }

    template<typename T> vector<T,2> atanh(const vector<T,2>& v) { return vector<T,2>(atanh(v.x), atanh(v.y)); }
    template<typename T> vector<T,3> atanh(const vector<T,3>& v) { return vector<T,3>(atanh(v.x), atanh(v.y), atanh(v.z)); }
    template<typename T> vector<T,4> atanh(const vector<T,4>& v) { return vector<T,4>(atanh(v.x), atanh(v.y), atanh(v.z), atanh(v.w)); }

    template<typename T> vector<T,2> cot(const vector<T,2>& v) { return vector<T,2>(cot(v.x), cot(v.y)); }
    template<typename T> vector<T,3> cot(const vector<T,3>& v) { return vector<T,3>(cot(v.x), cot(v.y), cot(v.z)); }
    template<typename T> vector<T,4> cot(const vector<T,4>& v) { return vector<T,4>(cot(v.x), cot(v.y), cot(v.z), cot(v.w)); }

    template<typename T> vector<T,2> pow(const vector<T,2>& v, const vector<T,2>& p) { return vector<T,2>(pow(v.x, p.x), pow(v.y, p.y)); }
    template<typename T> vector<T,3> pow(const vector<T,3>& v, const vector<T,3>& p) { return vector<T,3>(pow(v.x, p.x), pow(v.y, p.y), pow(v.z, p.z)); }
    template<typename T> vector<T,3> pow(const vector<T,4>& v, const vector<T,4>& p) { return vector<T,4>(pow(v.x, p.x), pow(v.y, p.y), pow(v.z, p.z), pow(v.w, p.w)); }
    template<typename T> vector<T,2> pow(const vector<T,2>& v, T p) { return vector<T,2>(pow(v.x, p), pow(v.y, p)); }
    template<typename T> vector<T,3> pow(const vector<T,3>& v, T p) { return vector<T,3>(pow(v.x, p), pow(v.y, p), pow(v.z, p)); }
    template<typename T> vector<T,4> pow(const vector<T,4>& v, T p) { return vector<T,4>(pow(v.x, p), pow(v.y, p), pow(v.z, p), pow(v.w, p)); }

    template<typename T> vector<T,2> exp(const vector<T,2>& v) { return vector<T,2>(exp(v.x), exp(v.y)); }
    template<typename T> vector<T,3> exp(const vector<T,3>& v) { return vector<T,3>(exp(v.x), exp(v.y), exp(v.z)); }
    template<typename T> vector<T,4> exp(const vector<T,4>& v) { return vector<T,4>(exp(v.x), exp(v.y), exp(v.z), exp(v.w)); }

    template<typename T> vector<T,2> log(const vector<T,2>& v) { return vector<T,2>(log(v.x), log(v.y)); }
    template<typename T> vector<T,3> log(const vector<T,3>& v) { return vector<T,3>(log(v.x), log(v.y), log(v.z)); }
    template<typename T> vector<T,4> log(const vector<T,4>& v) { return vector<T,4>(log(v.x), log(v.y), log(v.z), exp(v.w)); }

    template<typename T> vector<T,2> exp2(const vector<T,2>& v) { return vector<T,2>(exp2(v.x), exp2(v.y)); }
    template<typename T> vector<T,3> exp2(const vector<T,3>& v) { return vector<T,3>(exp2(v.x), exp2(v.y), exp2(v.z)); }
    template<typename T> vector<T,4> exp2(const vector<T,4>& v) { return vector<T,4>(exp2(v.x), exp2(v.y), exp2(v.z), exp2(v.w)); }

    template<typename T> vector<T,2> log2(const vector<T,2>& v) { return vector<T,2>(log2(v.x), log2(v.y)); }
    template<typename T> vector<T,3> log2(const vector<T,3>& v) { return vector<T,3>(log2(v.x), log2(v.y), log2(v.z)); }
    template<typename T> vector<T,4> log2(const vector<T,4>& v) { return vector<T,4>(log2(v.x), log2(v.y), log2(v.z), log2(v.w)); }

    template<typename T> vector<T,2> sqrt(const vector<T,2>& v) { return vector<T,2>(sqrt(v.x), sqrt(v.y)); }
    template<typename T> vector<T,3> sqrt(const vector<T,3>& v) { return vector<T,3>(sqrt(v.x), sqrt(v.y), sqrt(v.z)); }
    template<typename T> vector<T,4> sqrt(const vector<T,4>& v) { return vector<T,4>(sqrt(v.x), sqrt(v.y), sqrt(v.z), sqrt(v.w)); }

    template<typename T> vector<T,2> rsqrt(const vector<T,2>& v) { return vector<T,2>(rsqrt(v.x), rsqrt(v.y)); }
    template<typename T> vector<T,3> rsqrt(const vector<T,3>& v) { return vector<T,3>(rsqrt(v.x), rsqrt(v.y), rsqrt(v.z)); }
    template<typename T> vector<T,4> rsqrt(const vector<T,4>& v) { return vector<T,4>(rsqrt(v.x), rsqrt(v.y), rsqrt(v.z), rsqrt(v.w)); }

    template<typename T, int N> vector<T,2> rcp(const vector<T,N>& v) { return static_cast<T>(1) / v; }

    template<typename T> vector<T,2> abs(const vector<T,2>& v) { return vector<T,2>(abs(v.x), abs(v.y)); }
    template<typename T> vector<T,3> abs(const vector<T,3>& v) { return vector<T,3>(abs(v.x), abs(v.y), abs(v.z)); }
    template<typename T> vector<T,4> abs(const vector<T,4>& v) { return vector<T,4>(abs(v.x), abs(v.y), abs(v.z), abs(v.w)); }

    template<typename T> vector<T,2> round(const vector<T,2>& v) { return vector<T,2>(round(v.x), round(v.y)); }
    template<typename T> vector<T,3> round(const vector<T,3>& v) { return vector<T,3>(round(v.x), round(v.y), round(v.z)); }
    template<typename T> vector<T,4> round(const vector<T,4>& v) { return vector<T,4>(round(v.x), round(v.y), round(v.z), round(v.w)); }

    template<typename T> vector<T,2> ceil(const vector<T,2>& v) { return vector<T,2>(ceil(v.x), ceil(v.y)); }
    template<typename T> vector<T,3> ceil(const vector<T,3>& v) { return vector<T,3>(ceil(v.x), ceil(v.y), ceil(v.z)); }
    template<typename T> vector<T,4> ceil(const vector<T,4>& v) { return vector<T,4>(ceil(v.x), ceil(v.y), ceil(v.z), ceil(v.w)); }

    template<typename T> vector<T,2> floor(const vector<T,2>& v) { return vector<T,2>(floor(v.x), floor(v.y)); }
    template<typename T> vector<T,3> floor(const vector<T,3>& v) { return vector<T,3>(floor(v.x), floor(v.y), floor(v.z)); }
    template<typename T> vector<T,4> floor(const vector<T,4>& v) { return vector<T,4>(floor(v.x), floor(v.y), floor(v.z), floor(v.w)); }

    template<typename T> vector<T,2> frac(const vector<T,2>& v) { return vector<T,2>(frac(v.x), frac(v.y)); }
    template<typename T> vector<T,3> frac(const vector<T,3>& v) { return vector<T,3>(frac(v.x), frac(v.y), frac(v.z)); }
    template<typename T> vector<T,4> frac(const vector<T,4>& v) { return vector<T,4>(frac(v.x), frac(v.y), frac(v.z), frac(v.w)); }

    template<typename T> vector<T,2> mod(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(mod(a.x, b.x), mod(a.y, b.y)); }
    template<typename T> vector<T,3> mod(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(mod(a.x, b.x), mod(a.y, b.y), mod(a.z, b.z)); }
    template<typename T> vector<T,4> mod(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(mod(a.x, b.x), mod(a.y, b.y), mod(a.z, b.z), mod(a.w, b.w)); }
    template<typename T> vector<T,2> mod(const vector<T,2>& a, T b) { return vector<T,2>(mod(a.x, b), mod(a.y, b)); }
    template<typename T> vector<T,3> mod(const vector<T,3>& a, T b) { return vector<T,3>(mod(a.x, b), mod(a.y, b), mod(a.z, b)); }
    template<typename T> vector<T,4> mod(const vector<T,4>& a, T b) { return vector<T,4>(mod(a.x, b), mod(a.y, b), mod(a.z, b), mod(a.w, b)); }

    template<typename T> vector<T,2> fma(const vector<T,2>& a, const vector<T,2>& b, const vector<T,2>& c) { return vector<T,2>(fma(a.x, b.x, c.x), fma(a.y, b.y, c.y)); }
    template<typename T> vector<T,3> fma(const vector<T,3>& a, const vector<T,3>& b, const vector<T,3>& c) { return vector<T,3>(fma(a.x, b.x, c.x), fma(a.y, b.y, c.y), fma(a.z, b.z, c.z)); }
    template<typename T> vector<T,4> fma(const vector<T,4>& a, const vector<T,4>& b, const vector<T,4>& c) { return vector<T,4>(fma(a.x, b.x, c.x), fma(a.y, b.y, c.y), fma(a.z, b.z, c.z), fma(a.w, b.w, c.w)); }

    template<typename T> vector<T,2> align(const vector<T,2>& v, T a) { return vector<T,2>(align(v.x, a), align(v.y, a)); }
    template<typename T> vector<T,3> align(const vector<T,3>& v, T a) { return vector<T,3>(align(v.x, a), align(v.y, a), align(v.z, a)); }
    template<typename T> vector<T,4> align(const vector<T,4>& v, T a) { return vector<T,4>(align(v.x, a), align(v.y, a), align(v.z, a), align(v.w, a)); }

    template<typename T> vector<T,2> min(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(min(a.x, b.x), min(a.y, b.y)); }
    template<typename T> vector<T,3> min(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
    template<typename T> vector<T,4> min(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)); }
    template<typename T> vector<T,2> min(const vector<T,2>& a, T b) { return vector<T,2>(min(a.x, b), min(a.y, b)); }
    template<typename T> vector<T,3> min(const vector<T,3>& a, T b) { return vector<T,3>(min(a.x, b), min(a.y, b), min(a.z, b)); }
    template<typename T> vector<T,4> min(const vector<T,4>& a, T b) { return vector<T,4>(min(a.x, b), min(a.y, b), min(a.z, b), min(a.w, b)); }
    template<typename T> T min(T a, T b, T c) { return min(min(a, b), c); }
    template<typename T> T min(T a, T b, T c, T d) { return min(min(min(a, b), c), d); }

    template<typename T> vector<T,2> max(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(max(a.x, b.x), max(a.y, b.y)); }
    template<typename T> vector<T,3> max(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
    template<typename T> vector<T,4> max(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)); }
    template<typename T> vector<T,2> max(const vector<T,2>& a, T b) { return vector<T,2>(max(a.x, b), max(a.y, b)); }
    template<typename T> vector<T,3> max(const vector<T,3>& a, T b) { return vector<T,3>(max(a.x, b), max(a.y, b), max(a.z, b)); }
    template<typename T> vector<T,4> max(const vector<T,4>& a, T b) { return vector<T,4>(max(a.x, b), max(a.y, b), max(a.z, b), max(a.w, b)); }
    template<typename T> T max(T a, T b, T c) { return max(max(a, b), c); }
    template<typename T> T max(T a, T b, T c, T d) { return max(max(max(a, b), c), d); }

    template<typename T, int N> vector<T,N> clamp(const vector<T,N>& v, const vector<T,N>& mi, const vector<T,N>& ma) { return max(min(v, ma), mi); }
    template<typename T, int N> vector<T,N> clamp(const vector<T,N>& v, T mi, T ma) { return max(min(v, ma), mi); }

    template<typename T, int N> vector<T, N> saturate(const vector<T, N>& v) { return max(min(v, static_cast<T>(1)), static_cast<T>(0)); }

    template<typename T> T cmin(const vector<T,2>& v) { return min(v.x, v.y); }
    template<typename T> T cmin(const vector<T,3>& v) { return min(min(v.x, v.y), v.z); }
    template<typename T> T cmin(const vector<T,4>& v) { return min(min(min(v.x, v.y), v.z), v.w); }

    template<typename T> T cmax(const vector<T,2>& v) { return max(v.x, v.y); }
    template<typename T> T cmax(const vector<T,3>& v) { return max(max(v.x, v.y), v.z); }
    template<typename T> T cmax(const vector<T,4>& v) { return max(max(max(v.x, v.y), v.z), v.w); }

    template<typename T> constexpr T csum(const vector<T, 2>& v) { return v.x + v.y; }
    template<typename T> constexpr T csum(const vector<T, 3>& v) { return v.x + v.y + v.z; }
    template<typename T> constexpr T csum(const vector<T, 4>& v) { return v.x + v.y + v.z + v.w; }

    template<typename T, int N> constexpr T levels(const vector<T,N>& v) { auto r = cmin(v); return r > static_cast<T>(0) ? log2(r) + static_cast<T>(1) : static_cast<T>(0); }

    template<typename T, typename U> vector<T,2> lerp(const vector<T,2>& a, const vector<T,2>& b, const vector<U,2>& i) { return vector<T,2>(lerp(a.x, b.x, i.x), lerp(a.y, b.y, i.y)); }
    template<typename T, typename U> vector<T,3> lerp(const vector<T,3>& a, const vector<T,3>& b, const vector<U,3>& i) { return vector<T,3>(lerp(a.x, b.x, i.x), lerp(a.y, b.y, i.y), lerp(a.z, b.z, i.z)); }
    template<typename T, typename U> vector<T,4> lerp(const vector<T,4>& a, const vector<T,4>& b, const vector<U,4>& i) { return vector<T,4>(lerp(a.x, b.x, i.x), lerp(a.y, b.y, i.y), lerp(a.z, b.z, i.z), lerp(a.w, b.w, i.w)); }
    template<typename T, typename U> vector<T,2> lerp(const vector<T,2>& a, const vector<T,2>& b, U i) { return vector<T,2>(lerp(a.x, b.x, i), lerp(a.y, b.y, i)); }
    template<typename T, typename U> vector<T,3> lerp(const vector<T,3>& a, const vector<T,3>& b, U i) { return vector<T,3>(lerp(a.x, b.x, i), lerp(a.y, b.y, i), lerp(a.z, b.z, i)); }
    template<typename T, typename U> vector<T,4> lerp(const vector<T,4>& a, const vector<T,4>& b, U i) { return vector<T,4>(lerp(a.x, b.x, i), lerp(a.y, b.y, i), lerp(a.z, b.z, i), lerp(a.w, b.w, i)); }

    template<typename T> constexpr vector<T,2> sign(const vector<T,2>& v) { return vector<T,2>(sign(v.x), sign(v.y)); }
    template<typename T> constexpr vector<T,3> sign(const vector<T,3>& v) { return vector<T,3>(sign(v.x), sign(v.y), sign(v.z)); }
    template<typename T> constexpr vector<T,4> sign(const vector<T,4>& v) { return vector<T,4>(sign(v.x), sign(v.y), sign(v.z), sign(v.w)); }

    template<typename T> vector<T,2> smoothstep(const vector<T,2>& a, const vector<T,2>& b, const vector<T,2>& i) { return vector<T,2>(smoothstep(a.x, b.x, i.x), smoothstep(a.y, b.y, i.y)); }
    template<typename T> vector<T,3> smoothstep(const vector<T,3>& a, const vector<T,3>& b, const vector<T,3>& i) { return vector<T,3>(smoothstep(a.x, b.x, i.x), smoothstep(a.y, b.y, i.y), smoothstep(a.z, b.z, i.z)); }
    template<typename T> vector<T,4> smoothstep(const vector<T,4>& a, const vector<T,4>& b, const vector<T,4>& i) { return vector<T,4>(smoothstep(a.x, b.x, i.x), smoothstep(a.y, b.y, i.y), smoothstep(a.z, b.z, i.z), smoothstep(a.w, b.w, i.w)); }
    template<typename T> vector<T,2> smoothstep(T a, T b, const vector<T,2>& i) { return vector<T,2>(smoothstep(a, b, i.x), smoothstep(a, b, i.y)); }
    template<typename T> vector<T,3> smoothstep(T a, T b, const vector<T,3>& i) { return vector<T,3>(smoothstep(a, b, i.x), smoothstep(a, b, i.y), smoothstep(a, b, i.z)); }
    template<typename T> vector<T,4> smoothstep(T a, T b, const vector<T,4>& i) { return vector<T,4>(smoothstep(a, b, i.x), smoothstep(a, b, i.y), smoothstep(a, b, i.z), smoothstep(a, b, i.w)); }

    template<typename T> constexpr vector<T,2> step(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(step(a.x, b.x), step(a.y, b.y)); }
    template<typename T> constexpr vector<T,3> step(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(step(a.x, b.x), step(a.y, b.y), step(a.z, b.z)); }
    template<typename T> constexpr vector<T,4> step(const vector<T,4>& a, const vector<T,4>& b) { return vector<T,4>(step(a.x, b.x), step(a.y, b.y), step(a.z, b.z), step(a.w, b.w)); }
    template<typename T> constexpr vector<T,2> step(T a, const vector<T,2>& b) { return vector<T,2>(step(a, b.x), step(a, b.y)); }
    template<typename T> constexpr vector<T,3> step(T a, const vector<T,3>& b) { return vector<T,3>(step(a, b.x), step(a, b.y), step(a, b.z)); }
    template<typename T> constexpr vector<T,4> step(T a, const vector<T,4>& b) { return vector<T,4>(step(a, b.x), step(a, b.y), step(a, b.z), step(a, b.w)); }
    template<typename T> constexpr T dot(const vector<T,2>& a, const vector<T,2>& b) { return a.x * b.x + a.y * b.y; }
    template<typename T> constexpr T dot(const vector<T,3>& a, const vector<T,3>& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    template<typename T> constexpr T dot(const vector<T,4>& a, const vector<T,4>& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

    template<typename T, int N> T length(const vector<T,N>& v) { return sqrt(dot(v,v)); }
    template<typename T, int N> T distance(const vector<T, N>& a, const vector<T, N>& b) { return length(a - b); }
    template<typename T, int N> vector<T,N> normalize(const vector<T,N>& v) { return v / length(v); }
    template<typename T, int N> vector<T,N> safenormalize(const vector<T,N>& v) { float l = length(v); return v * (l == static_cast<T>(0) ? static_cast<T>(0) : rcp(l)); }
    
    template<typename T> vector<T,2> cross(const vector<T,2>& a, const vector<T,2>& b) { return vector<T,2>(a.x * b.y - b.x * a.y); }
    template<typename T> vector<T,3> cross(const vector<T,3>& a, const vector<T,3>& b) { return vector<T,3>(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y); }

    template<typename T, int N> vector<T,N> reflect(const vector<T,N>& i, const vector<T,N>& n) { return i - n * dot(n, i) * static_cast<T>(2); }
    
    template<typename T, int N> vector<T,N> refract(const vector<T,N>& i, const vector<T,N>& n, T eta) 
    { 
        const T dt = dot(n, i);
        const T k = static_cast<T>(1) - eta * eta * (static_cast<T>(1) - dt * dt);
        return (k >= static_cast<T>(0)) ? (eta * i - (eta * dt + sqrt(k)) * n) : vector<T,N>(0);
    }

    template<typename T> constexpr matrix<T,2,2> transpose(const matrix<T,2,2>& m) { return matrix<T,2,2>(m[0][0], m[1][0], m[0][1], m[1][1]); }
    template<typename T> constexpr matrix<T,3,2> transpose(const matrix<T,2,3>& m) { return matrix<T,3,2>(m[0][0], m[1][0], m[0][1], m[1][1], m[0][2], m[1][2]); }
    template<typename T> constexpr matrix<T,4,2> transpose(const matrix<T,2,4>& m) { return matrix<T,4,2>(m[0][0], m[1][0], m[0][1], m[1][1], m[0][2], m[1][2], m[0][3], m[1][3]); }
    template<typename T> constexpr matrix<T,2,3> transpose(const matrix<T,3,2>& m) { return matrix<T,2,3>(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1]); }
    template<typename T> constexpr matrix<T,3,3> transpose(const matrix<T,3,3>& m) { return matrix<T,3,3>(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]); }
    template<typename T> constexpr matrix<T,4,3> transpose(const matrix<T,3,4>& m) { return matrix<T,3,4>(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2], m[0][3], m[1][3], m[2][3]); }
    template<typename T> constexpr matrix<T,2,4> transpose(const matrix<T,4,2>& m) { return matrix<T,2,4>(m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1]); }
    template<typename T> constexpr matrix<T,3,4> transpose(const matrix<T,4,3>& m) { return matrix<T,3,4>(m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1], m[0][2], m[1][2], m[2][2], m[3][2]); }
    template<typename T> constexpr matrix<T,4,4> transpose(const matrix<T,4,4>& m) { return matrix<T,4,4>(m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1], m[0][2], m[1][2], m[2][2], m[3][2], m[0][3], m[1][3], m[2][3], m[3][3]); }
    template<typename T> constexpr matrix<T,3,4> transpose3x4(const matrix<T,4,4>& m) { return matrix<T,3,4>(m[0][0], m[1][0], m[2][0], m[3][0], m[0][1], m[1][1], m[2][1], m[3][1], m[0][2], m[1][2], m[2][2], m[3][2]); }
    template<typename T> constexpr matrix<T,4,4> transpose4x4(const matrix<T,3,4>& m) { return matrix<T,4,4>(m[0][0], m[1][0], m[2][0], static_cast<T>(0), m[0][1], m[1][1], m[2][1], static_cast<T>(0), m[0][2], m[1][2], m[2][2], static_cast<T>(0), m[0][3], m[1][3], m[2][3], static_cast<T>(1)); }

    template<typename T> constexpr T determinant(const matrix<T,2,2>& m) { return m[0][0] * m[1][1] - m[1][0] * m[0][1]; }
    template<typename T> constexpr T determinant(const matrix<T,3,3>& m) { return m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]) + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]); }
    template<typename T> constexpr T determinant(const matrix<T,4,4>& m)
    { 
        const auto f00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const auto f01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        const auto f02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        const auto f03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        const auto f04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        const auto f05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        const vector<T,4> coeff(+(m[1][1] * f00 - m[1][2] * f01 + m[1][3] * f02), -(m[1][0] * f00 - m[1][2] * f03 + m[1][3] * f04), +(m[1][0] * f01 - m[1][1] * f03 + m[1][3] * f05), -(m[1][0] * f02 - m[1][1] * f04 + m[1][2] * f05));
        return +m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]) + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]); 
    }


    template<typename T> matrix<T,2,2> inverse(const matrix<T,2,2>& m)
    {
        const auto invd = rcp(+m[0][0] * m[1][1] - m[1][0] * m[0][1]);
        return matrix<T,2,2>(+m[1][1] * invd, -m[0][1] * invd, -m[1][0] * invd, +m[0][0] * invd);
    }

    template<typename T> matrix<T,3,3> inverse(const matrix<T,3,3>& m)
    {
        const auto invd = rcp(+m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) - m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]) + m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]));
        return matrix<T, 3, 3>(
        +(m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invd,
        -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * invd,
        +(m[0][1] * m[1][2] - m[1][1] * m[0][2]) * invd,
        -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * invd,
        +(m[0][0] * m[2][2] - m[2][0] * m[0][2]) * invd,
        -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * invd,
        +(m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invd,
        -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invd,
        +(m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invd);
    }

    template<typename T> matrix<T,4,4> inverse(const matrix<T,4,4>& m)
    {
        const auto c00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const auto c02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        const auto c03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
        const auto c04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        const auto c06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        const auto c07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];
        const auto c08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        const auto c10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        const auto c11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
        const auto c12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        const auto c14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        const auto c15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];
        const auto c16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        const auto c18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        const auto c19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];
        const auto c20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        const auto c22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        const auto c23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        vector<T,4> f0(c00, c00, c02, c03);
        vector<T,4> f1(c04, c04, c06, c07);
        vector<T,4> f2(c08, c08, c10, c11);
        vector<T,4> f3(c12, c12, c14, c15);
        vector<T,4> f4(c16, c16, c18, c19);
        vector<T,4> f5(c20, c20, c22, c23);

        vector<T,4> v0(m[1][0], m[0][0], m[0][0], m[0][0]);
        vector<T,4> v1(m[1][1], m[0][1], m[0][1], m[0][1]);
        vector<T,4> v2(m[1][2], m[0][2], m[0][2], m[0][2]);
        vector<T,4> v3(m[1][3], m[0][3], m[0][3], m[0][3]);

        vector<T,4> i0(v1 * f0 - v2 * f1 + v3 * f2);
        vector<T,4> i1(v0 * f0 - v2 * f3 + v3 * f4);
        vector<T,4> i2(v0 * f1 - v1 * f3 + v3 * f5);
        vector<T,4> i3(v0 * f2 - v1 * f4 + v2 * f5);

        vector<T,4> sA(+1, -1, +1, -1);
        vector<T,4> sB(-1, +1, -1, +1);
        matrix<T,4,4> inv(i0 * sA, i1 * sB, i2 * sA, i3 * sB);
        vector<T,4> r0(inv[0][0], inv[1][0], inv[2][0], inv[3][0]);
        vector<T,4> d0(m[0] * r0);
        return inv * rcp((d0.x + d0.y) + (d0.z + d0.w));
    }

    
    template<typename T> matrix<T,3,3> affineInverse(const matrix<T,3,3>& m)
    {
        const matrix<T,2,2> inv(inverse(matrix<T,2,2>(m)));
        return matrix<T,3,3>(vector<T,3>(inv[0], static_cast<T>(0)), vector<T,3>(inv[1], static_cast<T>(0)), vector<T,3>(-inv * vector<T,2>(m[2]), static_cast<T>(1)));
    }

    template<typename T> matrix<T,4,4> affineInverse(const matrix<T,4,4>& m)
    {
        const matrix<T,3,3> inv(inverse(matrix<T,3,3>(m)));
        return matrix<T,4,4>(vector<T,4>(inv[0], static_cast<T>(0)), vector<T,4>(inv[1], static_cast<T>(0)), vector<T,4>(inv[2], static_cast<T>(0)), vector<T,4>(-inv * vector<T,3>(m[3]), static_cast<T>(1)));
    }


    template<typename T> matrix<T,2,2> adjugate(const matrix<T,2,2>& m) { return matrix<T,2,2>(+m[1][1], -m[1][0], -m[0][1], +m[0][0]); }

    template<typename T> matrix<T,3,3> adjugate(const matrix<T,3,3>& m)
    {
        return matrix<T,3,3>(
            +determinant(matrix<T,2,2>(m[1][1], m[2][1], m[1][2], m[2][2])),
            -determinant(matrix<T,2,2>(m[0][1], m[2][1], m[0][2], m[2][2])),
            +determinant(matrix<T,2,2>(m[0][1], m[1][1], m[0][2], m[1][2])),
            -determinant(matrix<T,2,2>(m[1][0], m[2][0], m[1][2], m[2][2])),
            +determinant(matrix<T,2,2>(m[0][0], m[2][0], m[0][2], m[2][2])),
            -determinant(matrix<T,2,2>(m[0][0], m[1][0], m[0][2], m[1][2])),
            +determinant(matrix<T,2,2>(m[1][0], m[2][0], m[1][1], m[2][1])),
            -determinant(matrix<T,2,2>(m[0][0], m[2][0], m[0][1], m[2][1])),
            +determinant(matrix<T,2,2>(m[0][0], m[1][0], m[0][1], m[1][1])));
    }

    template<typename T> matrix<T,4,4> adjugate(const matrix<T,4,4>& m)
    {
        return matrix<T,4,4>(
            +determinant(matrix<T,3,3>(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3])),
            -determinant(matrix<T,3,3>(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3])),
            +determinant(matrix<T,3,3>(m[1][0], m[1][1], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][1], m[3][3])),
            -determinant(matrix<T,3,3>(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2])),
            -determinant(matrix<T,3,3>(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3])),
            +determinant(matrix<T,3,3>(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3])),
            -determinant(matrix<T,3,3>(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3])),
            +determinant(matrix<T,3,3>(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2])),
            +determinant(matrix<T,3,3>(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3])),
            -determinant(matrix<T,3,3>(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3])),
            +determinant(matrix<T,3,3>(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3])),
            -determinant(matrix<T,3,3>(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2])),
            -determinant(matrix<T,3,3>(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3])),
            +determinant(matrix<T,3,3>(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3])),
            -determinant(matrix<T,3,3>(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3])),
            +determinant(matrix<T,3,3>(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2])));
    }
}
