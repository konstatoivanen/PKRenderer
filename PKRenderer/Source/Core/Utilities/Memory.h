#pragma once
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32) && defined(_WIN64)
    #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(_alloca(sizeof(Type) * count))
#else
    #define PK_STACK_ALLOC(Type, count) static_cast<Type*>(alloca(sizeof(Type) * count))
#endif

#if PK_DEBUG
    #define PK_DEBUG_ASSERT(cond, msg) PK::Memory::Assert(cond, msg);
#else
    #define PK_DEBUG_ASSERT(cond, msg)
#endif

namespace PK::Memory
{
    template<typename> inline constexpr bool IsLvalueRef = false;
    template<typename T> inline constexpr bool IsLvalueRef<T&> = true;

    template <typename> inline constexpr bool IsArray = false; 
    template <typename T, size_t size> inline constexpr bool IsArray<T[size]> = true;
    template <typename T> inline constexpr bool IsArray<T[]> = true;

    template<bool test, class T = void> struct EnableIf {};
    template<class T> struct EnableIf<true, T> { using Type = T; };
    template<bool test, class T = void> using EnableIf_T = typename EnableIf<test, T>::Type;

    template<typename T> struct RemoveRef { using Type = T; using ConstType = const T; };
    template<typename T> struct RemoveRef<T&> { using Type = T; using ConstType = const T&; };
    template<typename T> struct RemoveRef<T&&> { using Type = T; using ConstType = const T&&; };
    template<typename T> using RemoveRef_T = typename RemoveRef<T>::Type;

    template<typename T> struct RemoveCV { using Type = T; };
    template<typename T> struct RemoveCV<const T> { using Type = T; };
    template<typename T> struct RemoveCV<volatile T> { using Type = T; };
    template<typename T> struct RemoveCV<const volatile T> { using Type = T; };
    template<typename T> using RemoveCV_T = typename RemoveCV<T>::Type;

    template <typename T>
    [[nodiscard]] constexpr T&& Forward(RemoveRef_T<T>& v) noexcept { return static_cast<T&&>(v); }

    template <typename T>
    [[nodiscard]] constexpr T&& Forward(RemoveRef_T<T>&& v) noexcept { static_assert(!IsLvalueRef<T>, "Bad forward cast."); return static_cast<T&&>(v); }

    template <typename T>
    [[nodiscard]] constexpr RemoveRef_T<T>&& Move(T&& v) noexcept { return static_cast<RemoveRef_T<T>&&>(v); }

    template<typename T0, typename T1 = T0>
    constexpr T0 Exchange(T0& v, T1&& v_new) noexcept { T0 v_old = static_cast<T0&&>(v); v = static_cast<T1&&>(v_new); return v_old; }

    template<typename T>
    constexpr void Swap(T& a, T& b) noexcept { T t = Move(a); a = Move(b); b = Move(t); }

    void Assert(bool value, const char* str);
    void Assert(bool value);

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
    T* Memmove(T* dst, const T* src, size_t count) noexcept { return static_cast<T*>(memmove(dst, src, count * sizeof(T))); }

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
        if constexpr (__is_trivially_copyable(T))
        {
            Memmove<T>(dst, src, count);
        }
        else
        {
            for (auto i = 0u; i < count; ++i)
            {
                dst[i] = Move(src[i]);
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
                (values + i)->~T();
            }
        }

        Memset<T>(values, 0, count);
    }
}
