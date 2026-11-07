#pragma once
#include "Templates.h"

namespace PK::Hash
{
    constexpr static const uint64_t FNV1A_OFFSET_BASIS = 14695981039346656037ull;
    constexpr static const uint64_t FNV1A_PRIME = 1099511628211ull;

    uint32_t ExpandPrime(uint32_t oldSize);
    uint32_t ExpandSize(uint32_t capacity, uint32_t size);
    inline size_t ExpandPrime(size_t oldSize) { return ExpandPrime((uint32_t)oldSize); }
    inline size_t ExpandSize(size_t capacity, size_t size) { return ExpandSize((uint32_t)capacity, (uint32_t)size); }

    uint32_t ByteArray(const void* data, size_t size);
    uint64_t Murmur(const void* data, size_t size, uint64_t seed);
    uint64_t FNV1a(const void* data, size_t size, uint64_t hash = FNV1A_OFFSET_BASIS);
    uint64_t Interlace32x2(uint32_t a, uint32_t b);

    constexpr uint64_t Mix(uint64_t seed, uint64_t value)
    {
        seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6ull) + (seed >> 2ull);
        seed ^= seed >> 33ull;
        seed *= 0xff51afd7ed558ccdull;
        seed ^= seed >> 33ull;
        return seed;
    }

    template<typename T>
    inline size_t Get(const T& k) 
    {
        if constexpr (TIsPointer<T>)
        {
            // Reduce collisions by reducing type ptr offsets to a minimum.
            return reinterpret_cast<size_t>(k) / sizeof(typename TRemovePtr<T>::Type); 
        } 
        else if constexpr (TIsEnum<T>)
        {
            return (__underlying_type(T))k;
        }
        else
        {
            return FNV1a(&k, sizeof(T));
        }
    }

    template<> inline size_t Get(const uint8_t& k) { return k; }
    template<> inline size_t Get(const uint16_t& k) { return k; }
    template<> inline size_t Get(const uint32_t& k) { return k; }
    template<> inline size_t Get(const uint64_t& k) { return k; }
    
    template<> inline size_t Get(const int8_t& k) { return static_cast<unsigned>(k); }
    template<> inline size_t Get(const int16_t& k) { return static_cast<unsigned>(k); }
    template<> inline size_t Get(const int32_t& k) { return static_cast<unsigned>(k); }
    template<> inline size_t Get(const int64_t& k) { return static_cast<unsigned>(k); }

    template<> inline size_t Get(const float& k) { return *reinterpret_cast<const uint32_t*>(&k); }
    template<> inline size_t Get(const double& k) { return *reinterpret_cast<const uint64_t*>(&k); }

    template<typename T> struct TMurmur { size_t operator()(const T& k) const noexcept { return Hash::Murmur(&k, sizeof(T), 18446744073709551557ull); } };
    template<typename T> struct TFNV1a{ size_t operator()(const T& k) const noexcept { return Hash::FNV1a(&k, sizeof(T)); } };
    template<typename T> struct TCast{ size_t operator()(const T& k) const noexcept { return static_cast<size_t>(k); } };
    template<typename T> struct TPointer{ size_t operator()(const T* k) const noexcept { return reinterpret_cast<size_t>(k) / sizeof(T); } };
    template<typename T> struct THash { size_t operator()(const T& k) const noexcept { return Get<T>(k); } };
}
