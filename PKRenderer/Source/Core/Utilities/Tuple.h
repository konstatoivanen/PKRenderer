#pragma once
#include "Templates.h"

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
        using TupleBase<PK::TIndexSequenceFor<Args...>, Args...>::TupleBase;

        template<typename TFunc>
        static constexpr auto Dispatch(TFunc&& func) noexcept
        {
            return PK::Forward<TFunc>(func).template operator()<Args...>();
        }

        template <typename TFunc>
        static constexpr void For(TFunc&& func) noexcept
        {
            (func.template operator()<Args>(), ...);
        }
    };
    
    namespace Sequence
    {
        template <size_t N, typename T> constexpr T& DeriveAt(TupleElement<N, T>& t) noexcept { return t.value; }
        template <size_t N, typename T> constexpr const T& DeriveAt(const TupleElement<N, T>& t) noexcept { return t.value; }
        template <size_t N, typename T> constexpr volatile T& DeriveAt(volatile TupleElement<N, T>& t) noexcept { return t.value; }
        template <size_t N, typename T> constexpr const volatile T& DeriveAt(const volatile TupleElement<N, T>& t) noexcept { return t.value; }
        template <size_t N, typename T> constexpr T&& DeriveAt(TupleElement<N, T>&& t) noexcept { return PK::Forward<T>(t.value); }

        template <size_t N, typename ...T> constexpr decltype(auto) GetAt(Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template <size_t N, typename ...T> constexpr decltype(auto) GetAt(const Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template <size_t N, typename ...T> constexpr decltype(auto) GetAt(const volatile Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template <size_t N, typename ...T> constexpr decltype(auto) GetAt(volatile Tuple<T...>& t) noexcept { return DeriveAt<N>(t); }
        template <size_t N, typename ...T> constexpr decltype(auto) GetAt(Tuple<T...>&& t) noexcept { return DeriveAt<N>(PK::MoveTemp(t)); }

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
