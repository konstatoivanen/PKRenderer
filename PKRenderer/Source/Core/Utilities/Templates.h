#pragma once

namespace PK
{
    #if defined(__clang__)
    template<typename T0, typename T1> constexpr bool TIsSame = __is_same(T0, T1);
    #else
    template<typename, typename> constexpr bool TIsSame = false;
    template<typename T> constexpr bool TIsSame<T, T> = true;
    #endif 

    template<typename TFrom, typename TTo> inline constexpr bool TIsConvertible = __is_convertible_to(TFrom, TTo);

    template<typename> inline constexpr bool TIsArray = false;
    template<typename T, size_t N> inline constexpr bool TIsArray<T[N]> = true;
    template<typename T> inline constexpr bool TIsArray<T[]> = true;

    template<typename> inline constexpr bool TIsPointer = false; 
    template<typename T> inline constexpr bool TIsPointer<T*> = true;
    template<typename T> inline constexpr bool TIsPointer<T* const> = true;
    template<typename T> inline constexpr bool TIsPointer<T* volatile> = true;
    template<typename T> inline constexpr bool TIsPointer<T* const volatile> = true;

    template<typename T> inline constexpr bool TIsEnum = __is_enum(T);

    template <typename> constexpr bool TIsRValueRef = false; 
    template <typename T> constexpr bool TIsRValueRef<T&&> = true;

    template <class T, template <class...> class Template>
    inline constexpr bool TIsSpecialization = false;

    template <template <class...> class Template, class... Args>
    inline constexpr bool TIsSpecialization<Template<Args...>, Template> = true;

    template<bool predicate, typename T = void> struct TEnableIf;
    template<typename T> struct TEnableIf<true, T> { using Type = T; };
    template<typename T> struct TEnableIf<false, T> {};

    template<typename T> struct TRemoveRef { using Type = T; };
    template<typename T> struct TRemoveRef<T&> { using Type = T; };
    template<typename T> struct TRemoveRef<T&&> { using Type = T; };

    template<typename T> struct TRemovePtr { using Type = T; };
    template<typename T> struct TRemovePtr<T*> { using Type = T; };
    template<typename T> struct TRemovePtr<T* const> { using Type = T; };
    template<typename T> struct TRemovePtr<T* volatile> { using Type = T; };
    template<typename T> struct TRemovePtr<T* const volatile> { using Type = T; };

    template<typename T> struct TRemoveCV { using Type = T; };
    template<typename T> struct TRemoveCV<const T> { using Type = T; };
    template<typename T> struct TRemoveCV<volatile T> { using Type = T; };
    template<typename T> struct TRemoveCV<const volatile T> { using Type = T; };

    template<bool predicate, typename T = void> using TEnableIf_T = typename TEnableIf<predicate, T>::Type;
    template<typename T> using TRemoveRef_T = typename TRemoveRef<T>::Type;
    template<typename T> using TRemovePtr_T = typename TRemovePtr<T>::Type;
    template<typename T> using TRemoveCV_T = typename TRemoveCV<T>::Type;

    template<typename T, T N> struct TIntegerConstant { using Type = T; static constexpr T Value = N; };
    template<size_t Index> using TIndexConstant = TIntegerConstant<size_t, Index>;
    
    using TTrue = TIntegerConstant<bool, true>;
    using TFalse = TIntegerConstant<bool, false>;

    template<typename T, T... Values> struct TIntegerSequence { using Type = T; static constexpr size_t size() noexcept { return sizeof...(Values); } };
    template<typename T, T N> using TMakeIntegerSequence = __make_integer_seq<TIntegerSequence, T, N>;
    template<size_t... Values> using TIndexSequence = TIntegerSequence<size_t, Values...>;
    template<size_t N> using TMakeIndexSequence = TMakeIntegerSequence<size_t, N>;
    template<typename ... Types> using TIndexSequenceFor = TMakeIndexSequence<sizeof...(Types)>;

    struct TAny { TAny(size_t); template<typename T> constexpr operator T() const noexcept; };

    template<typename T, size_t n>
    static consteval bool TIsConstructible()
    {
        return[]<size_t... is>(PK::TIndexSequence<is...>) { return requires { T{ TAny(is)... }; }; } (PK::TMakeIndexSequence<n>());
    }

    template<typename T> [[nodiscard]] constexpr T&& Forward(TRemoveRef_T<T>& v) noexcept { return static_cast<T&&>(v); }
    template<typename T> [[nodiscard]] constexpr T&& Forward(TRemoveRef_T<T>&& v) noexcept { return static_cast<T&&>(v); }
    template<typename T> [[nodiscard]] constexpr TRemoveRef_T<T>&& MoveTemp(T&& v) noexcept { return static_cast<TRemoveRef_T<T>&&>(v); }

    template<typename T0, typename T1 = T0>
    constexpr T0 Exchange(T0& v, T1&& v_new) noexcept { T0 v_old = static_cast<T0&&>(v); v = static_cast<T1&&>(v_new); return v_old; }

    template<typename T>
    constexpr void Swap(T& a, T& b) noexcept { T t = MoveTemp(a); a = MoveTemp(b); b = MoveTemp(t); }
}
