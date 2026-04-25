#pragma once
#include <math.h>
#include <stdint.h>
#include <float.h>

namespace PK::math
{
    inline bool isnan(float v) { return ::isnan(v); }
    inline bool isnan(double v) { return ::isnan(v); }

    inline bool isinf(float v) { return ::isinf(v); }
    inline bool isinf(double v) { return ::isinf(v); }

    inline bool nearEqual(float x, float y, float e) { return ::fabsf(x - y) < e; }
    inline bool nearEqual(double x, double y, double e) { return ::fabs(x - y) < e; }

    inline float asfloat(uint32_t v) { union { uint32_t in; float out; } u; u.in = v; return u.out; }
    inline float asfloat(int32_t v) { union { uint32_t in; float out; } u; u.in = v; return u.out; }
    inline double asdouble(uint64_t v) { union { uint64_t in; double out; } u; u.in = v; return u.out; }
    inline double asdouble(int64_t v) { union { uint64_t in; double out; } u; u.in = v; return u.out; }

    inline int32_t asint(float v) { union { float in; int32_t out; } u; u.in = v; return u.out; }
    inline uint32_t asuint(float v) { union { float in; uint32_t out; } u; u.in = v; return u.out; }
    inline int64_t asint(double v) { union { double in; int64_t out; } u; u.in = v; return u.out; }
    inline uint64_t asuint(double v) { union { double in; uint64_t out; } u; u.in = v; return u.out; }

    inline float sin(float v) { return ::sinf(v); }
    inline double sin(double v) { return ::sin(v); }

    inline float cos(float v) { return ::cosf(v); }
    inline double cos(double v) { return ::cos(v); }
    
    inline float tan(float v) { return ::tanf(v); }
    inline double tan(double v) { return ::tan(v); }

    inline float asin(float v) { return ::asinf(v); }
    inline double asin(double v) { return ::asin(v); }

    inline float acos(float v) { return ::acosf(v); }
    inline double acos(double v) { return ::acos(v); }
    
    inline float atan(float v) { return ::atanf(v); }
    inline double atan(double v) { return ::atan(v); }

    inline float atan2(float a, float b) { return ::atan2f(a, b); }
    inline double atan2(double a, double b) { return ::atan2(a, b); }

    inline float sinh(float v) { return ::sinhf(v); }
    inline double sinh(double v) { return ::sinh(v); }

    inline float cosh(float v) { return ::coshf(v); }
    inline double cosh(double v) { return ::cosh(v); }
    
    inline float tanh(float v) { return ::tanhf(v); }
    inline double tanh(double v) { return ::tanh(v); }

    inline float asinh(float v) { return ::asinhf(v); }
    inline double asinh(double v) { return ::asinh(v); }

    inline float acosh(float v) { return ::acoshf(v); }
    inline double acosh(double v) { return ::acosh(v); }

    inline float atanh(float v) { return ::atanhf(v); }
    inline double atanh(double v) { return ::atanh(v); }

    inline float pow(float v, float p) { return ::powf(v,p); }
    inline double pow(double v, double p) { return ::pow(v,p); }
    
    inline float exp(float v) { return ::expf(v); }
    inline double exp(double v) { return ::exp(v); }
    
    inline float log(float v) { return ::logf(v); }
    inline double log(double v) { return ::log(v); }

    inline float exp2(float v) { return ::exp2f(v); }
    inline double exp2(double v) { return ::exp2(v); }
    
    inline float log2(float v) { return ::log2f(v); }
    inline double log2(double v) { return ::log2(v); }
    inline uint32_t log2(uint32_t v) { uint32_t y; asm("\tbsr %1, %0\n" : "=r"(y) : "r" (v)); return y; }
    inline uint64_t log2(uint64_t v) { return static_cast<uint64_t>(63 - __builtin_clzll(v)); }

    inline float sqrt(float v) { return ::sqrtf(v); }
    inline double sqrt(double v) { return ::sqrt(v); }
    
    inline float rsqrt(float v) { return 1.0f / ::sqrtf(v); }
    inline double rsqrt(double v) { return 1.0 / ::sqrt(v); }
    
    inline float abs(float v) { return ::fabsf(v); }
    inline double abs(double v) { return ::fabs(v); }
    inline int32_t abs(int32_t v) { return ::abs(v); }
    inline int64_t abs(int64_t v) { return ::llabs(v); }
    inline long abs(long v) { return ::labs(v); }
    
    inline float round(float v) { return ::roundf(v); }
    inline double round(double v) { return ::round(v); }

    inline float ceil(float v) { return ::ceilf(v); }
    inline double ceil(double v) { return ::ceil(v); }

    inline float floor(float v) { return ::floorf(v); }
    inline double floor(double v) { return ::floor(v); }

    inline float frac(float v) { return v - floor(v); }
    inline double frac(double v) { return v - floor(v); }

    inline float mod(float a, float b) { return ::fmodf(a, b); }
    inline double mod(double a, double b) { return ::fmod(a, b); }

    inline float fma(float a, float b, float c) { return ::fmaf(a, b, c); }
    inline double fma(double a, double b, double c) { return ::fma(a, b, c); }

    inline float min(float a, float b) { return ::fminf(a, b); }
    inline double min(double a, double b) { return ::fmin(a, b); }
    inline int8_t min(int8_t a, int8_t b) { return a < b ? a : b; }
    inline int16_t min(int16_t a, int16_t b) { return a < b ? a : b; }
    inline int32_t min(int32_t a, int32_t b) { return a < b ? a : b; }
    inline int64_t min(int64_t a, int64_t b) { return a < b ? a : b; }
    inline uint8_t min(uint8_t a, uint8_t b) { return a < b ? a : b; }
    inline uint16_t min(uint16_t a, uint16_t b) { return a < b ? a : b; }
    inline uint32_t min(uint32_t a, uint32_t b) { return a < b ? a : b; }
    inline uint64_t min(uint64_t a, uint64_t b) { return a < b ? a : b; }
    inline unsigned long min(unsigned long a, unsigned long b) { return a < b ? a : b; }
    inline long min(long a, long b) { return a < b ? a : b; }

    inline float max(float a, float b) { return ::fmaxf(a, b); }
    inline double max(double a, double b) { return ::fmax(a, b); }
    inline int8_t max(int8_t a, int8_t b) { return a > b ? a : b; }
    inline int16_t max(int16_t a, int16_t b) { return a > b ? a : b; }
    inline int32_t max(int32_t a, int32_t b) { return a > b ? a : b; }
    inline int64_t max(int64_t a, int64_t b) { return a > b ? a : b; }
    inline uint8_t max(uint8_t a, uint8_t b) { return a > b ? a : b; }
    inline uint16_t max(uint16_t a, uint16_t b) { return a > b ? a : b; }
    inline uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }
    inline uint64_t max(uint64_t a, uint64_t b) { return a > b ? a : b; }
    inline unsigned long max(unsigned long a, unsigned long b) { return a > b ? a : b; }
    inline long max(long a, long b) { return a > b ? a : b; }

    inline float clamp(float v, float mi, float ma) { return min(max(v, mi), ma); }
    inline double clamp(double v, double mi, double ma) { return min(max(v, mi), ma); }
    inline int8_t clamp(int8_t v, int8_t mi, int8_t ma) { return min(max(v, mi), ma); }
    inline int16_t clamp(int16_t v, int16_t mi, int16_t ma) { return min(max(v, mi), ma); }
    inline int32_t clamp(int32_t v, int32_t mi, int32_t ma) { return min(max(v, mi), ma); }
    inline int64_t clamp(int64_t v, int64_t mi, int64_t ma) { return min(max(v, mi), ma); }
    inline uint8_t clamp(uint8_t v, uint8_t mi, uint8_t ma) { return min(max(v, mi), ma); }
    inline uint16_t clamp(uint16_t v, uint16_t mi, uint16_t ma) { return min(max(v, mi), ma); }
    inline uint32_t clamp(uint32_t v, uint32_t mi, uint32_t ma) { return min(max(v, mi), ma); }
    inline uint64_t clamp(uint64_t v, uint64_t mi, uint64_t ma) { return min(max(v, mi), ma); }
    inline unsigned long clamp(unsigned long v, unsigned long mi, unsigned long ma) { return min(max(v, mi), ma); }
    inline long clamp(long v, long mi, long ma) { return min(max(v, mi), ma); }

    inline float saturate(float v) { return min(max(v, 0.0f), 1.0f); }
    inline double saturate(double v) { return min(max(v, 0.0), 1.0); }

    inline float lerp(float a, float b, float i) { return fmaf(i, b, (1.0f - i) * a); }
    inline double lerp(double a, double b, double i) { return fma(i, b, (1.0 - i) * a); }
    inline int8_t lerp(int8_t a, int8_t b, float i) { return (int8_t)(i * b + (1.0f - i) * a); }
    inline int16_t lerp(int16_t a, int16_t b, float i) { return (int16_t)(i * b + (1.0f - i) * a); }
    inline int32_t lerp(int32_t a, int32_t b, float i) { return (int32_t)(i * b + (1.0f - i) * a); }
    inline int64_t lerp(int64_t a, int64_t b, float i) { return (int64_t)(i * b + (1.0f - i) * a); }
    inline uint8_t lerp(uint8_t a, uint8_t b, float i) { return (uint8_t)(i * b + (1.0f - i) * a); }
    inline uint16_t lerp(uint16_t a, uint16_t b, float i) { return (uint16_t)(i * b + (1.0f - i) * a); }
    inline uint32_t lerp(uint32_t a, uint32_t b, float i) { return (uint32_t)(i * b + (1.0f - i) * a); }
    inline uint64_t lerp(uint64_t a, uint64_t b, float i) { return (uint64_t)(i * b + (1.0f - i) * a); }
    inline int8_t lerp(int8_t a, int8_t b, double i) { return (int8_t)(i * b + (1.0 - i) * a); }
    inline int16_t lerp(int16_t a, int16_t b, double i) { return (int16_t)(i * b + (1.0 - i) * a); }
    inline int32_t lerp(int32_t a, int32_t b, double i) { return (int32_t)(i * b + (1.0 - i) * a); }
    inline int64_t lerp(int64_t a, int64_t b, double i) { return (int64_t)(i * b + (1.0 - i) * a); }
    inline uint8_t lerp(uint8_t a, uint8_t b, double i) { return (uint8_t)(i * b + (1.0 - i) * a); }
    inline uint16_t lerp(uint16_t a, uint16_t b, double i) { return (uint16_t)(i * b + (1.0 - i) * a); }
    inline uint32_t lerp(uint32_t a, uint32_t b, double i) { return (uint32_t)(i * b + (1.0 - i) * a); }
    inline uint64_t lerp(uint64_t a, uint64_t b, double i) { return (uint64_t)(i * b + (1.0 - i) * a); }
    inline float lerp(float a, float b, bool i) { return i ? b : a; }
    inline double lerp(double a, double b, bool i) { return i ? b : a; }
    inline int8_t lerp(int8_t a, int8_t b, bool i) { return i ? b : a; }
    inline int16_t lerp(int16_t a, int16_t b, bool i) { return i ? b : a; }
    inline int32_t lerp(int32_t a, int32_t b, bool i) { return i ? b : a; }
    inline int64_t lerp(int64_t a, int64_t b, bool i) { return i ? b : a; }
    inline uint8_t lerp(uint8_t a, uint8_t b, bool i) { return i ? b : a; }
    inline uint16_t lerp(uint16_t a, uint16_t b, bool i) { return i ? b : a; }
    inline uint32_t lerp(uint32_t a, uint32_t b, bool i) { return i ? b : a; }
    inline uint64_t lerp(uint64_t a, uint64_t b, bool i) { return i ? b : a; }

    inline float sign(float v) { return (v > 0) - (v < 0); }
    inline double sign(double v) { return (v > 0) - (v < 0); }
    inline int8_t sign(int8_t v) { return (v > 0) - (v < 0); }
    inline int16_t sign(int16_t v) { return (v > 0) - (v < 0); }
    inline int32_t sign(int32_t v) { return (v > 0) - (v < 0); }
    inline int64_t sign(int64_t v) { return (v > 0ll) - (v < 0ll); }
    inline long sign(long v) { return (v > 0l) - (v < 0l); }

    inline float smoothstep(float a, float b, float i) { auto t = saturate((i - a) / (b - a)); return t * t * (3.0f - 2.0f * t); }
    inline double smoothstep(double a, double b, double i) { auto t = saturate((i - a) / (b - a)); return t * t * (3.0 - 2.0 * t); }

    inline float step(float a, float b) { return lerp(1.0f, 0.0f, b < a); }
    inline double step(double a, double b) { return lerp(1.0f, 0.0f, b < a); }

    inline uint16_t f32tof16(float v) 
    {
        const auto u0 = asuint(v);
        const auto u1 = u0 & 0x7FFFF000u;
        const auto u2 = (asuint(min(asfloat(u1) * 1.92592994e-34f, 260042752.0f)) + 0x1000u) >> 13u;
        return (u1 >= 0x7f800000u ? u1 > 0x7f800000u ? 0x7e00u : 0x7c00u : u2) | (u0 & ~0x7FFFF000u) >> 16u;
    }

    inline float f16tof32(uint16_t x) 
    {
        uint32_t uf = (x & 0x7fffu) << 13u;
        uint32_t e = uf & 0xf800000u;
        uf += e == 0xf800000u ? 0x70000000u : 0x38000000u;
        uf = e == 0 ? asuint(asfloat(uf + (1 << 23)) - 6.10351563e-05f) : uf;
        return asfloat(uf | (x & 0x8000) << 16);
    }
}
