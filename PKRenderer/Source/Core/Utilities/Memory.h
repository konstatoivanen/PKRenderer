#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <new>
#if !defined(PK_SYSTEM_ERROR)
#include <assert.h>
#endif
#include "Templates.h"

namespace PK::Memory
{
    // Fallback to these if no user defines have been set.
    #if !defined(PK_SYSTEM_DEFAULT_ALIGN)
        #define PK_SYSTEM_DEFAULT_ALIGN 16
    #endif

    #if !defined(PK_SYSTEM_ERROR)
        #define PK_SYSTEM_ERROR(message) assert(false)
    #endif
    
    #if !defined(PK_SYSTEM_ALIGNED_ALLOC)
    #if defined(_WIN32) && defined(_WIN64)
        #define PK_SYSTEM_ALIGNED_ALLOC(size, align) _aligned_malloc(size, align)
    #else
        #define PK_SYSTEM_ALIGNED_ALLOC(size, align) aligned_alloc(align, size)
    #endif
    #endif
    
    #if !defined(PK_SYSTEM_ALIGNED_FREE)
    #if defined(_WIN32) && defined(_WIN64)
        #define PK_SYSTEM_ALIGNED_FREE(ptr) _aligned_free(ptr)
    #else
        #define PK_SYSTEM_ALIGNED_FREE(ptr) free(ptr)
    #endif
    #endif

    #if defined(_WIN32) && defined(_WIN64)
        #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(_alloca(sizeof(Type) * count))
    #else
        #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(alloca(sizeof(Type) * count))
    #endif

    inline void Assert(bool value, [[maybe_unused]] const char* str) { if (!value) { PK_SYSTEM_ERROR(str); } }
    inline void Assert(bool value) { Assert(value, "Assertion failed!"); }

    inline void Free(void* block) noexcept { PK_SYSTEM_ALIGNED_FREE(block); }

    inline void* AllocateAligned(size_t size, size_t align = PK_SYSTEM_DEFAULT_ALIGN) noexcept { return PK_SYSTEM_ALIGNED_ALLOC(size, align); }

    template<typename T>
    T* Allocate(size_t count) noexcept { return static_cast<T*>(PK_SYSTEM_ALIGNED_ALLOC(count * sizeof(T), PK_SYSTEM_DEFAULT_ALIGN)); }

    // Slower than calloc but allows for aligned alloc.
    template<typename T>
    T* AllocateClear(size_t count) noexcept { return Memset<T>(Allocate<T>(count), 0, count); }

    
    template<typename T, typename ... Args>
    T* New(Args&& ... args) { return Construct<T>(Allocate<T>(1u), PK::Forward<Args>(args)...); }

    template<typename T>
    void Delete(T* ptr) { if (ptr) Free(Destruct<T>(ptr)); }


    template<typename T, typename ... Args>
    T* Construct(T* ptr, Args&& ... args) { return new(ptr) T(PK::Forward<Args>(args)...); }

    template<typename T, typename ... Args>
    T* Construct(void* ptr, Args&& ... args) { return new(ptr) T(PK::Forward<Args>(args)...); }

    template<typename T>
    T* Destruct(T* ptr) { ptr->~T(); return ptr; }

    template<typename T, typename ... Args>
    T* ConstructArray(T* ptr, size_t count, Args&& ... args) 
    { 
        for (auto i = 0u; i < count; ++i)
        {
            Construct(ptr + i, PK::Forward<Args>(args)...);
        }

        return ptr;
    }

    template<typename T>
    T* DestructArray(T* ptr, size_t count) 
    { 
        for (auto i = 0u; i < count; ++i)
        {
            (ptr + i)->~T(); 
        }
        
        return ptr; 
    }


    template<typename T>
    T* Memcpy(T* dst, const T* src, size_t count) noexcept { return static_cast<T*>(memcpy(dst, src, count * sizeof(T))); }

    template<typename T>
    T* Memmove(T* dst, const T* src, size_t count) noexcept { return static_cast<T*>(memmove(dst, src, count * sizeof(T))); }

    template<typename T>
    T* Memset(T* dst, int value, size_t count) noexcept { return static_cast<T*>(memset(dst, value, count * sizeof(T))); }


    template<typename T>
    void MoveArray(T* dst, T* src, size_t count)
    {
        if constexpr (__is_trivially_copyable(T))
        {
            Memmove<T>(dst, src, count);
        }
        else
        {
            for (auto i = 0u; i < count; ++i)
            {
                dst[i] = PK::MoveTemp(src[i]);
            }
        }
    }

    template<typename T>
    void CopyArray(T* dst, const T* src, size_t count)
    {
        if constexpr (__is_trivially_copyable(T))
        {
            Memmove<T>(dst, src, count);
        }
        else
        {
            for (auto i = 0u; i < count; ++i)
            {
                dst[i] = src[i];
            }
        }
    }

    template<typename T>
    void ClearArray(T* values, size_t count)
    {
        // Call Destructor & zero memory. Dont call constructor to avoid allocs.
        // A bit hazardous but whatever.
        if constexpr (!__is_trivially_copyable(T))
        {
            for (auto i = 0u; i < count; ++i)
            {
                Destruct(values + i);
            }
        }

        Memset<T>(values, 0, count);
    }

    template<typename T0, typename T1>
    T1 BitCast(const T0& value) { static_assert(sizeof(T0) == sizeof(T1)); T1 ret; memcpy(&ret, &value, sizeof(T0)); return ret; }

    template<typename T0, typename T1>
    T1 BitCast(const T0* ptr) noexcept { T1 ret; memcpy(&ret, ptr, sizeof(T1)); return ret; }


    template<typename TAlignment>
    size_t AlignSize(size_t size) noexcept { return size == 0ull ? 0ull : (size + sizeof(TAlignment) - 1ull) & ~(sizeof(TAlignment) - 1ull); }

    template<typename T>
    T* CastOffsetPtr(void* data, size_t offset) noexcept { return reinterpret_cast<T*>(static_cast<char*>(data) + offset); }
}
