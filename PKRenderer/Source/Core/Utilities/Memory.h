#pragma once
#include <stdlib.h>
#include <exception>

namespace PK::Memory
{
#if defined(_WIN32) && defined(_WIN64)
    #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(_alloca(sizeof(Type) * count))
#else
    #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(alloca(sizeof(Type) * count))
#endif

    #define PK_CONTAINER_RANGE_CHECK(index, min, max)                        \
    do                                                                       \
    {                                                                        \
        if (index >= max || index < min)                                     \
        {                                                                    \
            throw std::exception("Index/Count outside of container bounds!");\
        }                                                                    \
    }                                                                        \
    while(0)                                                                 \
                                                                             \

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


    template<typename TAlignment>
    size_t AlignSize(size_t* size)
    {
        if (*size == 0ull)
        {
            return *size;
        }

        *size = (*size + sizeof(TAlignment) - 1ull) & ~(sizeof(TAlignment) - 1ull);
        return *size;
    }

    template<typename T>
    T* CastOffsetPtr(void* data, size_t offset)
    {
        return reinterpret_cast<T*>(static_cast<char*>(data) + offset);
    }

    template<typename T>
    void MoveArray(T* dst, T* src, size_t count)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy(dst, src, sizeof(T) * count);
        }
        else
        {
            for (auto i = 0u; i < count; ++i)
            {
                dst[i] = std::move(src[i]);
            }
        }
    }

    template<typename T>
    void CopyArray(T* dst, T* src, size_t count)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy(dst, src, sizeof(T) * count);
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
        if constexpr (!std::is_trivially_copyable_v<T>)
        {
            for (auto i = 0u; i < count; ++i)
            {
                (values + i)->~T();
            }
        }

        memset(values, 0, sizeof(T) * count);
    }
}
