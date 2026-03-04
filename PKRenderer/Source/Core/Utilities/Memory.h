#pragma once
#include <stdlib.h>

namespace PK::Memory
{
#if defined(_WIN32) && defined(_WIN64)
    #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(_alloca(sizeof(Type) * count))
#else
    #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(alloca(sizeof(Type) * count))
#endif

    template<typename T>
    T* Malloc(size_t count) noexcept { return static_cast<T*>(malloc(count * sizeof(T))); }

    template<typename T>
    T* Calloc(size_t count) noexcept { return static_cast<T*>(calloc(count, sizeof(T))); }

    template<typename T>
    T* Realloc(T* block, size_t count) noexcept { return static_cast<T*>(realloc(block, count * sizeof(T))); }

    template<typename T>
    T* ReallocOrCalloc(T* block, size_t count, bool calloc) noexcept { return calloc ? Calloc<T>(count) : Realloc<T>(block, count); }

    template<typename T>
    T* Memcpy(T* dst, const T* src, size_t count) noexcept { return static_cast<T*>(memcpy(dst, src, count * sizeof(T))); }

    template<typename T>
    T* Memset(T* dst, int value, size_t count) noexcept { return static_cast<T*>(memset(dst, value, count * sizeof(T))); }

    template<typename T0, typename T1>
    T1 BitCast(const T0& value)
    {
        static_assert(sizeof(T0) == sizeof(T1));
        T1 ret;
        memcpy(&ret, &value, sizeof(T0));
        return ret;
    }

    template<typename T0, typename T1>
    T1 BitCast(const T0* ptr)
    {
        T1 ret;
        memcpy(&ret, ptr, sizeof(T1));
        return ret;
    }
}
