#pragma once
#include "Memory.h"

namespace PK
{
    template<size_t size>
    struct AllocationFixed
    {
        template<typename T> 
        struct alignas(16) Data 
        { 
            alignas(T) uint8_t buffer[sizeof(T) * size];
        };

        template<typename T> 
        constexpr static size_t GetCount([[maybe_unused]] const Data<T>& data) { return size; }
        
        template<typename T> 
        constexpr static size_t GetSize([[maybe_unused]] const Data<T>& data) { return sizeof(T) * size; }

        template<typename T> 
        inline static T* GetPtr(Data<T>& data) { return reinterpret_cast<T*>(&data.buffer); }
        
        template<typename T> 
        inline static T const* GetPtr(const Data<T>& data) { return reinterpret_cast<T const*>(&data.buffer); }
        
        template<typename T> 
        inline static Data<T> Allocate(size_t newSize)
        { 
            Memory::Assert(newSize <= size, "Fixed allocation type size exceeded!");
            return {};
        }

        template<typename T> 
        inline static void Free(Data<T>& data) {}
    };

    template<size_t inline_size>
    struct AllocationInline
    {
        template<typename T>
        struct alignas(16) Data
        {
            constexpr static const size_t inline_element_count = (inline_size > 0ull ? sizeof(T) * inline_size : sizeof(T*)) / sizeof(T);
            union { T* ptr; alignas(T) uint8_t inl[inline_size > 0ull ? sizeof(T) * inline_size : sizeof(T*)]; };
            size_t size;
        };

        template<typename T>
        constexpr static bool IsInlineSize(size_t size) { return size <= Data<T>::inline_element_count; }

        template<typename T>
        constexpr static size_t GetCount(const Data<T>& data) { return IsInlineSize<T>(data.size) ? Data<T>::inline_element_count : data.size; }

        template<typename T>
        constexpr static size_t GetSize(const Data<T>& data) { return sizeof(T) * GetCount<T>(data); }

        template<typename T>
        inline static T* GetPtr(Data<T>& data) { return IsInlineSize<T>(data.size) ? reinterpret_cast<T*>(&data) : data.ptr; }

        template<typename T>
        inline static T const* GetPtr(const Data<T>& data) { return IsInlineSize<T>(data.size) ? reinterpret_cast<T const*>(&data) : data.ptr; }

        template<typename T> 
        inline static Data<T> Allocate(size_t newSize)
        {
            if (IsInlineSize<T>(newSize))
            {
                return {};
            }

            Data<T> data;
            data.ptr = Memory::AllocateClear<T>(newSize);
            data.size = newSize;
            return data;
        }

        template<typename T>
        inline static void Free(Data<T>& data)
        {
            if (!IsInlineSize<T>(data.size))
            {
                Memory::Free(data.ptr);
                data.ptr = nullptr;
            }

            data.size = 0ull;
        }
    };

    struct AllocationHeap
    {
        template<typename T>
        struct Data
        {
            T* ptr; 
            size_t size;
        };

        template<typename T>
        constexpr static size_t GetCount(const Data<T>& data) { return data.size; }

        template<typename T>
        constexpr static size_t GetSize(const Data<T>& data) { return sizeof(T) * data.size; }

        template<typename T>
        inline static T* GetPtr(Data<T>& data) { return data.ptr; }

        template<typename T>
        inline static T const* GetPtr(const Data<T>& data) { return data.ptr; }

        template<typename T> 
        inline static Data<T> Allocate(size_t newSize) { return { Memory::AllocateClear<T>(newSize), newSize }; }

        template<typename T>
        inline static void Free(Data<T>& data)
        {
            Memory::Free(data.ptr);
            data.ptr = nullptr;
            data.size = 0ull;
        }
    };
}
