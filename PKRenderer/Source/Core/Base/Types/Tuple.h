#pragma once
#include "Core/Base/Templates.h"

namespace PK
{
    template <typename T0, typename T1>
    struct Pair
    {
        T0 first;
        T1 second;
    };

    template<size_t N, typename T> struct TupleElement { T value; };
    template<typename I, typename ... Args> struct TupleBase;
    template<> struct TupleBase<PK::TIndexSequence<>> {};

    template <size_t ... Index, typename ...Args>
    struct TupleBase<PK::TIndexSequence<Index...>, Args ... > : TupleElement<Index, Args>...
    {
        constexpr TupleBase() = default;
        constexpr TupleBase(TupleBase&&) = default;
        constexpr TupleBase(const TupleBase&) = default;
        constexpr TupleBase(Args&&... args) noexcept : TupleElement<Index, Args>{ static_cast<Args&&>(args) }... {}
    };

    template <typename ... Args>
    struct Tuple : TupleBase<PK::TIndexSequenceFor<Args...>, Args...>
    {
        constexpr const static size_t Size = sizeof...(Args);
        constexpr const static size_t Stride = (sizeof(Args) + ... + 0ull);
        using TupleBase<PK::TIndexSequenceFor<Args...>, Args...>::TupleBase;
        template<typename TFunc> constexpr static auto Dispatch(TFunc&& func) noexcept { return PK::Forward<TFunc>(func).template operator()<Args...>(); }
        template<typename TFunc> constexpr static void For(TFunc&& func) noexcept { (func.template operator()<Args>(), ...); }
    };

    template <typename T, typename TAccum, template<typename> typename TFilter>
    struct TFilterTuple;

    template <typename TAccum, template<typename> typename TFilter>
    struct TFilterTuple<Tuple<>, TAccum, TFilter>
    {
        using Type = TAccum;
    };

    template <typename T, typename... Args, typename... TAccum, template<typename> typename TFilter>
    struct TFilterTuple<Tuple<T, Args...>, Tuple<TAccum...>, TFilter> {

        using TNext = typename TConditional<TFilter<T>::value, Tuple<TAccum..., typename TFilter<T>::Type>, Tuple<TAccum...>>::Type;
        using Type = typename TFilterTuple<Tuple<Args...>, TNext, TFilter>::Type;
    };

    template <typename T>
    struct TTupleFilterRemoveCVRef
    {
        constexpr static bool value = true;
        using Type = TRemoveCVRef_T<T>;
    };

    template <typename T>
    struct TTupleFilterOnlyPtr
    {
        constexpr static bool value = TIsPointer<T>;
        using Type = typename TRemovePtr<T>::Type;
    };

    template <typename T>
    struct TTupleFilterAddPtr
    {
        constexpr static bool value = true;
        using Type = T*;
    };

    namespace Sequence
    {
        template<size_t N, typename T> constexpr T& DeriveAt(TupleElement<N, T>& t) noexcept { return t.value; }
        template<size_t N, typename T> constexpr const T& DeriveAt(const TupleElement<N, T>& t) noexcept { return t.value; }
        template<size_t N, typename T> constexpr volatile T& DeriveAt(volatile TupleElement<N, T>& t) noexcept { return t.value; }
        template<size_t N, typename T> constexpr const volatile T& DeriveAt(const volatile TupleElement<N, T>& t) noexcept { return t.value; }
        template<size_t N, typename T> constexpr T&& DeriveAt(TupleElement<N, T>&& t) noexcept { return PK::Forward<T>(t.value); }

        template<size_t N, typename ...T> constexpr decltype(auto) GetAt(Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template<size_t N, typename ...T> constexpr decltype(auto) GetAt(const Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template<size_t N, typename ...T> constexpr decltype(auto) GetAt(const volatile Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template<size_t N, typename ...T> constexpr decltype(auto) GetAt(volatile Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template<size_t N, typename ...T> constexpr decltype(auto) GetAt(Tuple<T...>&& t) noexcept { return DeriveAt<N>(PK::MoveTemp(t)); }

        template <typename T, typename... Args>
        consteval size_t GetTypeIndex() noexcept
        {
            auto index = 0ull;
            auto found = false;
            ((found ? 0 : (TIsSame<T,Args> ? (found = true, 0) : (++index, 0))), ...);
            return found ? index : -1ull;
        }

        template <typename T, typename... Args>
        constexpr T* GetTypePtr(Tuple<Args...>& tuple) noexcept
        {
            constexpr auto index = GetTypeIndex<T, Args...>();
            static_assert(index != -1ull, "Could not find type in tuple!");
            return &GetAt<index>(tuple);
        }

        template <typename T, typename... Args>
        constexpr const T* GetTypePtr(const Tuple<Args...>& tuple) noexcept
        {
            constexpr auto index = GetTypeIndex<T, Args...>();
            static_assert(index != -1ull, "Could not find type in tuple!");
            return &GetAt<index>(tuple);
        }

        template<size_t N, typename T> using TypeAt = TRemoveCVRef_T<decltype(DeriveAt<N>(*static_cast<T*>(nullptr)))>;

        template<typename T> using RemoveCVRef = typename TFilterTuple<T, Tuple<>, TTupleFilterRemoveCVRef>::Type;
        template<typename T> using OnlyPtr = typename TFilterTuple<T, Tuple<>, TTupleFilterOnlyPtr>::Type;
        template<typename T> using AddPtr = typename TFilterTuple<T, Tuple<>, TTupleFilterAddPtr>::Type;

        template <typename ... Args> constexpr auto Make(Args&& ... args) noexcept { return Tuple<TDecay_T<Args>...>{ PK::Forward<Args>(args)... }; }
        template <typename ... Args> constexpr auto Bind(Args& ... args) noexcept { return Tuple<Args&...>{ args ... }; }

        template <typename TFunc, typename... Args>
        constexpr decltype(auto) Dispatch(TFunc&& function, Tuple<Args...>& tuple) noexcept
        {
            return []<size_t... I>(auto&& f, auto& t, PK::TIndexSequence<I...>) noexcept -> decltype(auto)
            {
                return PK::Forward<TFunc>(f)(GetAt<I>(t)...);
            }
            (PK::Forward<TFunc>(function), tuple, PK::TIndexSequenceFor<Args...>{});
        }

        template <typename TFunc, typename... Args>
        constexpr void For(TFunc&& function, Tuple<Args...>& tuple) noexcept
        {
            [] <size_t... I>(auto&& f, auto& t, PK::TIndexSequence<I...>)
            {
                (f(GetAt<I>(t)), ...);
            }
            (PK::Forward<TFunc>(function), tuple, PK::TIndexSequenceFor<Args...>{});
        }
    }
}
