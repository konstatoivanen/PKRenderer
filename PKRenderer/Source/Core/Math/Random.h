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

    void setSeed(pcg32_state* rng, uint64_t state, uint64_t sequence);
    void setSeed(uint64_t state, uint64_t sequence);

    inline void setSeed(pcg32_state* rng, uint64_t seed) { setSeed(rng, 0x853c49e6748fea9bull, seed); }
    inline void setSeed(uint64_t seed) { setSeed(0x853c49e6748fea9bull, seed); }

    uint32_t randomUint(pcg32_state* rng);
    uint32_t randomUint();
    
    inline uint8_t randomByte() { return static_cast<uint8_t>(randomUint()); }
    inline uint16_t randomUshort() { return static_cast<uint16_t>(randomUint()); }
    inline uint64_t randomUlong() { return static_cast<uint64_t>(randomUint()) | (static_cast<uint64_t>(randomUint()) << 32ull); }
    inline int8_t randomSbyte() { return static_cast<uint8_t>(randomByte()); }
    inline int16_t randomShort() { return static_cast<int16_t>(randomUshort()); }
    inline int32_t randomInt() { return static_cast<int32_t>(randomUint()); }
    inline int64_t randomLong() { return static_cast<int64_t>(randomUlong()); }
    inline float randomFloat() { return asfloat((randomUint() & 0x007fffffu) | 0x3f800000u) - 1.0f; }
    inline double randomDouble() { return asdouble((randomUlong() & 0x000fffffffffffffull) | 0x3ff0000000000000ull) - 1.0; }

    inline vector<uint8_t,2> randomByte2() { return vector<uint8_t,2>(randomByte(), randomByte()); }
    inline vector<uint8_t,3> randomByte3() { return vector<uint8_t,3>(randomByte(), randomByte(), randomByte()); }
    inline vector<uint8_t,4> randomByte4() { return vector<uint8_t,4>(randomByte(), randomByte(), randomByte(), randomByte()); }

    inline vector<uint16_t,2> randomUshort2() { return vector<uint16_t,2>(randomUshort(), randomUshort()); }
    inline vector<uint16_t,3> randomUshort3() { return vector<uint16_t,3>(randomUshort(), randomUshort(), randomUshort()); }
    inline vector<uint16_t,4> randomUshort4() { return vector<uint16_t,4>(randomUshort(), randomUshort(), randomUshort(), randomUshort()); }
    
    inline vector<uint32_t,2> randomUint2() { return vector<uint32_t,2>(randomUint(), randomUint()); }
    inline vector<uint32_t,3> randomUint3() { return vector<uint32_t,3>(randomUint(), randomUint(), randomUint()); }
    inline vector<uint32_t,4> randomUint4() { return vector<uint32_t,4>(randomUint(), randomUint(), randomUint(), randomUint()); }
    
    inline vector<uint64_t,2> randomUlong2() { return vector<uint64_t,2>(randomUlong(), randomUlong()); }
    inline vector<uint64_t,3> randomUlong3() { return vector<uint64_t,3>(randomUlong(), randomUlong(), randomUlong()); }
    inline vector<uint64_t,4> randomUlong4() { return vector<uint64_t,4>(randomUlong(), randomUlong(), randomUlong(), randomUlong()); }

    inline vector<int8_t,2> randomSbyte2() { return vector<int8_t,2>(randomSbyte(), randomSbyte()); }
    inline vector<int8_t,3> randomSbyte3() { return vector<int8_t,3>(randomSbyte(), randomSbyte(), randomSbyte()); }
    inline vector<int8_t,4> randomSbyte4() { return vector<int8_t,4>(randomSbyte(), randomSbyte(), randomSbyte(), randomSbyte()); }

    inline vector<int16_t,2> randomShort2() { return vector<int16_t,2>(randomShort(), randomShort()); }
    inline vector<int16_t,3> randomShort3() { return vector<int16_t,3>(randomShort(), randomShort(), randomShort()); }
    inline vector<int16_t,4> randomShort4() { return vector<int16_t,4>(randomShort(), randomShort(), randomShort(), randomShort()); }
   
    inline vector<int32_t,2> randomInt2() { return vector<int32_t,2>(randomInt(), randomInt()); }
    inline vector<int32_t,3> randomInt3() { return vector<int32_t,3>(randomInt(), randomInt(), randomInt()); }
    inline vector<int32_t,4> randomInt4() { return vector<int32_t,4>(randomInt(), randomInt(), randomInt(), randomInt()); }
    
    inline vector<int64_t,2> randomLong2() { return vector<int64_t,2>(randomLong(), randomLong()); }
    inline vector<int64_t,3> randomLong3() { return vector<int64_t,3>(randomLong(), randomLong(), randomLong()); }
    inline vector<int64_t,4> randomLong4() { return vector<int64_t,4>(randomLong(), randomLong(), randomLong(), randomLong()); }
   
    inline vector<float,2> randomFloat2() { return vector<float,2>(randomFloat(), randomFloat()); }
    inline vector<float,3> randomFloat3() { return vector<float,3>(randomFloat(), randomFloat(), randomFloat()); }
    inline vector<float,4> randomFloat4() { return vector<float,4>(randomFloat(), randomFloat(), randomFloat(), randomFloat()); }

    inline vector<double,2> randomDouble2() { return vector<double,2>(randomDouble(), randomDouble()); }
    inline vector<double,3> randomDouble3() { return vector<double,3>(randomDouble(), randomDouble(), randomDouble()); }
    inline vector<double,4> randomDouble4() { return vector<double,4>(randomDouble(), randomDouble(), randomDouble(), randomDouble()); }

    inline uint8_t randomRange(uint8_t mi, uint8_t ma) { return static_cast<uint8_t>((randomUint() - mi) % (ma - mi + 1u)); }
    inline uint16_t randomRange(uint16_t mi, uint16_t ma) { return static_cast<uint16_t>((randomUint() - mi) % (ma - mi + 1u)); }
    inline uint32_t randomRange(uint32_t mi, uint32_t ma) { return static_cast<uint32_t>((randomUint() - mi) % (ma - mi + 1u)); }
    inline uint64_t randomRange(uint64_t mi, uint64_t ma) { return static_cast<uint64_t>((randomUlong() - mi) % (ma - mi + 1ull)); }
    inline int8_t randomRange(int8_t mi, int8_t ma) { return static_cast<int8_t>((randomInt() - mi) % (ma - mi + 1)); }
    inline int16_t randomRange(int16_t mi, int16_t ma) { return static_cast<int16_t>((randomInt() - mi) % (ma - mi + 1)); }
    inline int32_t randomRange(int32_t mi, int32_t ma) { return static_cast<int32_t>((randomInt() - mi) % (ma - mi + 1)); }
    inline int64_t randomRange(int64_t mi, int64_t ma) { return static_cast<int64_t>((randomLong() - mi) % (ma - mi + 1ll)); }
    inline float randomRange(float mi, float ma) { return fma(randomFloat(), ma - mi, mi); }
    inline double randomRange(double mi, double ma) { return fma(randomDouble(), ma - mi, mi); }

    template<typename T> inline vector<T,2> randomRange(const vector<T,2>& mi, const vector<T,2>& ma) { return vector<T,2>(randomRange(mi.x, ma.x), randomRange(mi.y, ma.y)); }
    template<typename T> inline vector<T,3> randomRange(const vector<T,3>& mi, const vector<T,3>& ma) { return vector<T,3>(randomRange(mi.x, ma.x), randomRange(mi.y, ma.y), randomRange(mi.z, ma.z)); }
    template<typename T> inline vector<T,4> randomRange(const vector<T,4>& mi, const vector<T,4>& ma) { return vector<T,4>(randomRange(mi.x, ma.x), randomRange(mi.y, ma.y), randomRange(mi.z, ma.z), randomRange(mi.w, ma.w)); }

    inline float randomEulerFloat() { return randomFloat() * 360.0f; }
    inline vector<float,2> randomEulerFloat2() { return vector<float,2>(randomEulerFloat(), randomEulerFloat()); }
    inline vector<float,3> randomEulerFloat3() { return vector<float,3>(randomEulerFloat(), randomEulerFloat(), randomEulerFloat()); }
    inline vector<float,4> randomEulerFloat4() { return vector<float,4>(randomEulerFloat(), randomEulerFloat(), randomEulerFloat(), randomEulerFloat()); }

    inline float randomRadianFloat() { return randomFloat() * 2.0f * PK_FLOAT_PI; }
    inline vector<float,2> randomRadianFloat2() { return vector<float,2>(randomRadianFloat(), randomRadianFloat()); }
    inline vector<float,3> randomRadianFloat3() { return vector<float,3>(randomRadianFloat(), randomRadianFloat(), randomRadianFloat()); }
    inline vector<float,4> randomRadianFloat4() { return vector<float,4>(randomRadianFloat(), randomRadianFloat(), randomRadianFloat(), randomRadianFloat()); }

    inline double randomEulerDouble() { return randomDouble() * 360.0; }
    inline vector<double,2> randomEulerDouble2() { return vector<double,2>(randomEulerDouble(), randomEulerDouble()); }
    inline vector<double,3> randomEulerDouble3() { return vector<double,3>(randomEulerDouble(), randomEulerDouble(), randomEulerDouble()); }
    inline vector<double,4> randomEulerDouble4() { return vector<double,4>(randomEulerDouble(), randomEulerDouble(), randomEulerDouble(), randomEulerDouble()); }

    inline double randomRadianDouble() { return randomDouble() * 2.0 * PK_FLOAT_PI; }
    inline vector<double,2> randomRadianDouble2() { return vector<double,2>(randomRadianDouble(), randomRadianDouble()); }
    inline vector<double,3> randomRadianDouble3() { return vector<double,3>(randomRadianDouble(), randomRadianDouble(), randomRadianDouble()); }
    inline vector<double,4> randomRadianDouble4() { return vector<double,4>(randomRadianDouble(), randomRadianDouble(), randomRadianDouble(), randomRadianDouble()); }

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
