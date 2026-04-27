#pragma once
#include "Math.h"

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */

namespace PK::math
{
    struct pcg32_state 
    { 
        uint64_t state = 0x853c49e6748fea9bULL;
        uint64_t inc = 0xda3e39cb94b95bdbULL;
    };
 
    extern thread_local pcg32_state s_thread_pcg_state;

    void setseed(pcg32_state* rng, uint64_t state, uint64_t sequence);
    void setseed(uint64_t state, uint64_t sequence);

    inline void setseed(pcg32_state* rng, uint64_t seed) { setseed(rng, 0x853c49e6748fea9bull, seed); }
    inline void setseed(uint64_t seed) { setseed(0x853c49e6748fea9bull, seed); }

    uint32_t random_uint(pcg32_state* rng);
    uint32_t random_uint();
    
    inline uint8_t random_byte() { return static_cast<uint8_t>(random_uint()); }
    inline uint16_t random_ushort() { return static_cast<uint16_t>(random_uint()); }
    inline uint64_t random_ulong() { return static_cast<uint64_t>(random_uint()) | (static_cast<uint64_t>(random_uint()) << 32ull); }
    inline int8_t random_sbyte() { return static_cast<uint8_t>(random_byte()); }
    inline int16_t random_short() { return static_cast<int16_t>(random_ushort()); }
    inline int32_t random_int() { return static_cast<int32_t>(random_uint()); }
    inline int64_t random_long() { return static_cast<int64_t>(random_ulong()); }
    inline float random_float() { return asfloat((random_uint() & 0x007fffffu) | 0x3f800000u) - 1.0f; }
    inline double random_double() { return asdouble((random_ulong() & 0x000fffffffffffffull) | 0x3ff0000000000000ull) - 1.0; }

    inline vector<uint8_t,2> random_byte2() { return vector<uint8_t,2>(random_byte(), random_byte()); }
    inline vector<uint8_t,3> random_byte3() { return vector<uint8_t,3>(random_byte(), random_byte(), random_byte()); }
    inline vector<uint8_t,4> random_byte4() { return vector<uint8_t,4>(random_byte(), random_byte(), random_byte(), random_byte()); }

    inline vector<uint16_t,2> random_ushort2() { return vector<uint16_t,2>(random_ushort(), random_ushort()); }
    inline vector<uint16_t,3> random_ushort3() { return vector<uint16_t,3>(random_ushort(), random_ushort(), random_ushort()); }
    inline vector<uint16_t,4> random_ushort4() { return vector<uint16_t,4>(random_ushort(), random_ushort(), random_ushort(), random_ushort()); }
    
    inline vector<uint32_t,2> random_uint2() { return vector<uint32_t,2>(random_uint(), random_uint()); }
    inline vector<uint32_t,3> random_uint3() { return vector<uint32_t,3>(random_uint(), random_uint(), random_uint()); }
    inline vector<uint32_t,4> random_uint4() { return vector<uint32_t,4>(random_uint(), random_uint(), random_uint(), random_uint()); }
    
    inline vector<uint64_t,2> random_ulong2() { return vector<uint64_t,2>(random_ulong(), random_ulong()); }
    inline vector<uint64_t,3> random_ulong3() { return vector<uint64_t,3>(random_ulong(), random_ulong(), random_ulong()); }
    inline vector<uint64_t,4> random_ulong4() { return vector<uint64_t,4>(random_ulong(), random_ulong(), random_ulong(), random_ulong()); }

    inline vector<int8_t,2> random_sbyte2() { return vector<int8_t,2>(random_sbyte(), random_sbyte()); }
    inline vector<int8_t,3> random_sbyte3() { return vector<int8_t,3>(random_sbyte(), random_sbyte(), random_sbyte()); }
    inline vector<int8_t,4> random_sbyte4() { return vector<int8_t,4>(random_sbyte(), random_sbyte(), random_sbyte(), random_sbyte()); }

    inline vector<int16_t,2> random_short2() { return vector<int16_t,2>(random_short(), random_short()); }
    inline vector<int16_t,3> random_short3() { return vector<int16_t,3>(random_short(), random_short(), random_short()); }
    inline vector<int16_t,4> random_short4() { return vector<int16_t,4>(random_short(), random_short(), random_short(), random_short()); }
   
    inline vector<int32_t,2> random_int2() { return vector<int32_t,2>(random_int(), random_int()); }
    inline vector<int32_t,3> random_int3() { return vector<int32_t,3>(random_int(), random_int(), random_int()); }
    inline vector<int32_t,4> random_int4() { return vector<int32_t,4>(random_int(), random_int(), random_int(), random_int()); }
    
    inline vector<int64_t,2> random_long2() { return vector<int64_t,2>(random_long(), random_long()); }
    inline vector<int64_t,3> random_long3() { return vector<int64_t,3>(random_long(), random_long(), random_long()); }
    inline vector<int64_t,4> random_long4() { return vector<int64_t,4>(random_long(), random_long(), random_long(), random_long()); }
   
    inline vector<float,2> random_float2() { return vector<float,2>(random_float(), random_float()); }
    inline vector<float,3> random_float3() { return vector<float,3>(random_float(), random_float(), random_float()); }
    inline vector<float,4> random_float4() { return vector<float,4>(random_float(), random_float(), random_float(), random_float()); }

    inline vector<double,2> random_doubl2() { return vector<double,2>(random_double(), random_double()); }
    inline vector<double,3> random_doubl3() { return vector<double,3>(random_double(), random_double(), random_double()); }
    inline vector<double,4> random_doubl4() { return vector<double,4>(random_double(), random_double(), random_double(), random_double()); }

    inline uint8_t random_range(uint8_t mi, uint8_t ma) { return static_cast<uint8_t>((random_uint() - mi) % (ma - mi + 1u)); }
    inline uint16_t random_range(uint16_t mi, uint16_t ma) { return static_cast<uint16_t>((random_uint() - mi) % (ma - mi + 1u)); }
    inline uint32_t random_range(uint32_t mi, uint32_t ma) { return static_cast<uint32_t>((random_uint() - mi) % (ma - mi + 1u)); }
    inline uint64_t random_range(uint64_t mi, uint64_t ma) { return static_cast<uint64_t>((random_ulong() - mi) % (ma - mi + 1ull)); }
    inline int8_t random_range(int8_t mi, int8_t ma) { return static_cast<int8_t>((random_int() - mi) % (ma - mi + 1)); }
    inline int16_t random_range(int16_t mi, int16_t ma) { return static_cast<int16_t>((random_int() - mi) % (ma - mi + 1)); }
    inline int32_t random_range(int32_t mi, int32_t ma) { return static_cast<int32_t>((random_int() - mi) % (ma - mi + 1)); }
    inline int64_t random_range(int64_t mi, int64_t ma) { return static_cast<int64_t>((random_long() - mi) % (ma - mi + 1ll)); }
    inline float random_range(float mi, float ma) { return fma(random_float(), ma - mi, mi); }
    inline double random_range(double mi, double ma) { return fma(random_double(), ma - mi, mi); }

    template<typename T> inline vector<T,2> random_range(const vector<T,2>& mi, const vector<T,2>& ma) { return vector<T,2>(random_range(mi.x, ma.x), random_range(mi.y, ma.y)); }
    template<typename T> inline vector<T,3> random_range(const vector<T,3>& mi, const vector<T,3>& ma) { return vector<T,3>(random_range(mi.x, ma.x), random_range(mi.y, ma.y), random_range(mi.z, ma.z)); }
    template<typename T> inline vector<T,4> random_range(const vector<T,4>& mi, const vector<T,4>& ma) { return vector<T,4>(random_range(mi.x, ma.x), random_range(mi.y, ma.y), random_range(mi.z, ma.z), random_range(mi.w, ma.w)); }

    inline float random_euler_float() { return random_float() * 360.0f; }
    inline vector<float,2> random_euler_float2() { return vector<float,2>(random_euler_float(), random_euler_float()); }
    inline vector<float,3> random_euler_float3() { return vector<float,3>(random_euler_float(), random_euler_float(), random_euler_float()); }
    inline vector<float,4> random_euler_float4() { return vector<float,4>(random_euler_float(), random_euler_float(), random_euler_float(), random_euler_float()); }

    inline float random_radian_float() { return random_float() * 2.0f * PK_FLOAT_PI; }
    inline vector<float,2> random_radian_float2() { return vector<float,2>(random_radian_float(), random_radian_float()); }
    inline vector<float,3> random_radian_float3() { return vector<float,3>(random_radian_float(), random_radian_float(), random_radian_float()); }
    inline vector<float,4> random_radian_float4() { return vector<float,4>(random_radian_float(), random_radian_float(), random_radian_float(), random_radian_float()); }

    inline double random_euler_double() { return random_double() * 360.0; }
    inline vector<double,2> random_euler_double2() { return vector<double,2>(random_euler_double(), random_euler_double()); }
    inline vector<double,3> random_euler_double3() { return vector<double,3>(random_euler_double(), random_euler_double(), random_euler_double()); }
    inline vector<double,4> random_euler_double4() { return vector<double,4>(random_euler_double(), random_euler_double(), random_euler_double(), random_euler_double()); }

    inline double random_radian_double() { return random_double() * 2.0 * PK_FLOAT_PI; }
    inline vector<double,2> random_radian_double2() { return vector<double,2>(random_radian_double(), random_radian_double()); }
    inline vector<double,3> random_radian_double3() { return vector<double,3>(random_radian_double(), random_radian_double(), random_radian_double()); }
    inline vector<double,4> random_radian_double4() { return vector<double,4>(random_radian_double(), random_radian_double(), random_radian_double(), random_radian_double()); }

    float halton(uint32_t index, uint32_t radix);
    inline vector<float,2> halton(uint32_t index, const vector<uint32_t,2>& radix) { return vector<float,2>(halton(index, radix.x), halton(index, radix.y)); }
    inline vector<float,3> halton(uint32_t index, const vector<uint32_t,3>& radix) { return vector<float,3>(halton(index, radix.x), halton(index, radix.y), halton(index, radix.z)); }
    inline vector<float,4> halton(uint32_t index, const vector<uint32_t,4>& radix) { return vector<float,4>(halton(index, radix.x), halton(index, radix.y), halton(index, radix.z), halton(index, radix.w)); }

    vector<float,2> hammersley(uint32_t i, uint32_t n);

    uint32_t murmurhash11(uint32_t seed);
    vector<uint32_t,2> murmurhash21(uint32_t seed);
    vector<uint32_t,3> murmurhash31(uint32_t seed);
    vector<uint32_t,4> murmurhash41(uint32_t seed);
}
