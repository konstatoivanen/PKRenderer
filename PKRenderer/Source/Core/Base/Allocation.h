#pragma once
#include "Memory.h"

namespace PK
{
    template<typename T, size_t N, size_t C>
    inline constexpr size_t pk_align_inline_16() 
    { 
        constexpr const auto size = (N > 0ull ? sizeof(T) * N : sizeof(T*)) + C;
        constexpr const auto aligned = ((size + 15ull) / 16ull) * 16ull;
        return aligned - C;
    }

    template<size_t size>
    struct AllocationFixed
    {
        constexpr static const bool is_fixed = true;

        template<typename T> 
        struct alignas(16) Data 
        { 
            alignas(T) uint8_t buffer[pk_align_inline_16<T, size, 0>()];
        
            constexpr static size_t GetCount([[maybe_unused]] const Data& data) { return size; }
            constexpr static size_t GetSize([[maybe_unused]] const Data& data) { return sizeof(T) * size; }
            inline static T* GetPtr(Data& data) { return reinterpret_cast<T*>(&data.buffer); }
            inline static T const* GetPtr(const Data& data) { return reinterpret_cast<T const*>(&data.buffer); }
            
            inline static Data Allocate(size_t newSize)
            {
                Memory::Assert(newSize <= size, "Fixed allocation type size exceeded!");
                return {};
            }

            inline static void Free([[maybe_unused]] Data<T>& data) {}
        };
    };

    template<size_t inline_size>
    struct AllocationInline
    {
        constexpr static const bool is_fixed = false;

        template<typename T>
        struct alignas(16) Data
        {
            union { T* ptr; alignas(T) uint8_t inl[pk_align_inline_16<T, inline_size, 8>()]; };
            size_t size;

            constexpr static const size_t inline_element_count = sizeof(inl) / sizeof(T);
            constexpr static size_t GetCount(const Data& data) { return data.size <= inline_element_count ? inline_element_count : data.size; }
            constexpr static size_t GetSize(const Data& data) { return sizeof(T) * GetCount(data); }
            inline static T* GetPtr(Data& data) { return data.size <= inline_element_count ? reinterpret_cast<T*>(&data) : data.ptr; }
            inline static T const* GetPtr(const Data& data) { return data.size <= inline_element_count ? reinterpret_cast<T const*>(&data) : data.ptr; }

            inline static Data Allocate(size_t newSize)
            {
                if (newSize > inline_element_count)
                {
                    Data data;
                    data.ptr = Memory::AllocateClear<T>(newSize);
                    data.size = newSize;
                    return data;
                }

                return {};
            }

            inline static void Free(Data& data)
            {
                if (data.size > inline_element_count)
                {
                    Memory::Free(data.ptr);
                    data.ptr = nullptr;
                }

                data.size = 0ull;
            }
        };
    };

    struct AllocationHeap
    {
        constexpr static const bool is_fixed = false;

        template<typename T>
        struct Data
        {
            T* ptr; 
            size_t size;
       
            constexpr static size_t GetCount(const Data& data) { return data.size; }
            constexpr static size_t GetSize(const Data& data) { return sizeof(T) * data.size; }
            inline static T* GetPtr(Data& data) { return data.ptr; }
            inline static T const* GetPtr(const Data& data) { return data.ptr; }
            
            inline static Data Allocate(size_t newSize) 
            { 
                return { Memory::AllocateClear<T>(newSize), newSize }; 
            }
            
            inline static void Free(Data& data) 
            {
                Memory::Free(data.ptr);
                data.ptr = nullptr;
                data.size = 0ull;
            }
        };
    };
}
