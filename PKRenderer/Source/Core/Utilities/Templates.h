#pragma once

namespace PK
{
    template <typename>                inline constexpr bool TIsArray = false;
    template <typename T, size_t size> inline constexpr bool TIsArray<T[size]> = true;
    template <typename T>              inline constexpr bool TIsArray<T[]> = true;

    template <typename>   inline constexpr bool TIsPointer = false; 
    template <typename T> inline constexpr bool TIsPointer<T*> = true;
    template <typename T> inline constexpr bool TIsPointer<T* const> = true;
    template <typename T> inline constexpr bool TIsPointer<T* volatile> = true;
    template <typename T> inline constexpr bool TIsPointer<T* const volatile> = true;

    template <typename T> inline constexpr bool TIsEnum = __is_enum(T);

    template<bool predicate, typename T = void> struct TEnableIf;
    template<typename T>                        struct TEnableIf<true, T> { using Type = T; };
    template<typename T>                        struct TEnableIf<false, T> {};

    template<typename T> struct TRemoveRef      { using Type = T; };
    template<typename T> struct TRemoveRef<T&>  { using Type = T; };
    template<typename T> struct TRemoveRef<T&&> { using Type = T; };

    template<typename T> struct TRemovePtr                      { using Type = T; };
    template<typename T> struct TRemovePtr<T*>                  { using Type = T; };
    template<typename T> struct TRemovePtr<T* const>            { using Type = T; };
    template<typename T> struct TRemovePtr<T* volatile>         { using Type = T; };
    template<typename T> struct TRemovePtr<T* const volatile>   { using Type = T; };

    template<typename T> struct TRemoveCV                   { using Type = T; };
    template<typename T> struct TRemoveCV<const T>          { using Type = T; };
    template<typename T> struct TRemoveCV<volatile T>       { using Type = T; };
    template<typename T> struct TRemoveCV<const volatile T> { using Type = T; };

    template<bool predicate, typename T = void> using TEnableIf_T = typename TEnableIf<predicate, T>::Type;
    template<typename T>                        using TRemoveRef_T = typename TRemoveRef<T>::Type;
    template<typename T>                        using TRemovePtr_T = typename TRemovePtr<T>::Type;
    template<typename T>                        using TRemoveCV_T = typename TRemoveCV<T>::Type;

    template <typename T>
    [[nodiscard]] constexpr T&& Forward(TRemoveRef_T<T>& v) noexcept { return static_cast<T&&>(v); }

    template <typename T>
    [[nodiscard]] constexpr T&& Forward(TRemoveRef_T<T>&& v) noexcept { return static_cast<T&&>(v); }

    template <typename T>
    [[nodiscard]] constexpr TRemoveRef_T<T>&& MoveTemp(T&& v) noexcept { return static_cast<TRemoveRef_T<T>&&>(v); }

    template<typename T0, typename T1 = T0>
    constexpr T0 Exchange(T0& v, T1&& v_new) noexcept { T0 v_old = static_cast<T0&&>(v); v = static_cast<T1&&>(v_new); return v_old; }

    template<typename T>
    constexpr void Swap(T& a, T& b) noexcept { T t = MoveTemp(a); a = MoveTemp(b); b = MoveTemp(t); }
}
