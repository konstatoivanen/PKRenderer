#pragma once
#include <stdint.h>
#include "Templates.h"

namespace PK::Hash
{
    bool IsPrime(int32_t candidate);
    int32_t GetPrime(int32_t min);
    uint32_t ExpandPrime(uint32_t oldSize);
    uint32_t ExpandSize(uint32_t capacity, uint32_t size);

    uint32_t ByteArrayHash(const void* data, size_t count);
    uint64_t MurmurHash(const void* data, size_t count, uint64_t seed);
    uint64_t FNV1AHash(const void* data, size_t count);
    uint64_t InterlaceHash32x2(uint32_t a, uint32_t b);

    template<typename T>
    inline size_t GetHash(const T& k) 
    {
        if constexpr (TIsPointer<T>)
        {
            // Reduce collisions by reducing type ptr offsets to a minimum.
            return reinterpret_cast<size_t>(k) / sizeof(typename TRemovePtr<T>::Type); 
        } 
        else if constexpr (TIsEnum<T>)
        {
            return (size_t)((__underlying_type(T))k);
        }
        else
        {
            return FNV1AHash(&k, sizeof(T));
        }
    }

    template<> inline size_t GetHash(const uint8_t& k) { return (size_t)k; }
    template<> inline size_t GetHash(const uint16_t& k) { return (size_t)k; }
    template<> inline size_t GetHash(const uint32_t& k) { return (size_t)k; }
    template<> inline size_t GetHash(const uint64_t& k) { return (size_t)k; }
    
    template<> inline size_t GetHash(const int8_t& k) { return (size_t)k; }
    template<> inline size_t GetHash(const int16_t& k) { return (size_t)k; }
    template<> inline size_t GetHash(const int32_t& k) { return (size_t)k; }
    template<> inline size_t GetHash(const int64_t& k) { return (size_t)k; }

    template<> inline size_t GetHash(const float& k) { return (size_t)(*reinterpret_cast<const uint32_t*>(&k)); }
    template<> inline size_t GetHash(const double& k) { return (size_t)(*reinterpret_cast<const uint64_t*>(&k)); }

    template<typename T> struct TMurmurHash { size_t operator()(const T& k) const noexcept { return Hash::MurmurHash(&k, sizeof(T), 18446744073709551557ull); } };
    template<typename T> struct TFNV1AHash  { size_t operator()(const T& k) const noexcept { return Hash::FNV1AHash(&k, sizeof(T)); } };
    template<typename T> struct TCastHash { size_t operator()(const T& k) const noexcept { return (size_t)(k); } };
    template<typename T> struct TPointerHash { size_t operator()(const T* k) const noexcept { return reinterpret_cast<size_t>(k) / sizeof(T); } };
    template<typename T> struct THash { size_t operator()(const T& k) const noexcept { return GetHash<T>(k); } };
}
