#pragma once
#include "Templates.h"

namespace PK
{
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
    };
    
    template <size_t N, typename T> constexpr T& TupleDeriveElement(TupleElement<N, T>& t) noexcept { return t.value; }
    template <size_t N, typename T> constexpr const T& TupleDeriveElement(const TupleElement<N, T>& t) noexcept { return t.value; }
    template <size_t N, typename T> constexpr volatile T& TupleDeriveElement(volatile TupleElement<N, T>& t) noexcept { return t.value; }
    template <size_t N, typename T> constexpr const volatile T& TupleDeriveElement(const volatile TupleElement<N, T>& t) noexcept { return t.value; }
    template <size_t N, typename T> constexpr T&& TupleDeriveElement(TupleElement<N, T>&& t) noexcept { return PK::Forward<T>(t.value); }

    template <size_t N, typename ...T> constexpr decltype(auto) TupleGetElement(Tuple<T...>& t) noexcept { return TupleDeriveElement<N>(t); }
    template <size_t N, typename ...T> constexpr decltype(auto) TupleGetElement(const Tuple<T...>& t) noexcept { return TupleDeriveElement<N>(t); }
    template <size_t N, typename ...T> constexpr decltype(auto) TupleGetElement(const volatile Tuple<T...>& t) noexcept { return TupleDeriveElement<N>(t); }
    template <size_t N, typename ...T> constexpr decltype(auto) TupleGetElement(volatile Tuple<T...>& t) noexcept { return TupleDeriveElement<N>(t); }
    template <size_t N, typename ...T> constexpr decltype(auto) TupleGetElement(Tuple<T...>&& t) noexcept { return TupleDeriveElement<N>(PK::MoveTemp(t)); }

    template <typename ... Args> constexpr auto TupleMake(Args& ... args) noexcept { return Tuple<Args&...>{ args ... }; }
    template <typename ... Args> constexpr auto TupleForward(Args&& ... args) noexcept { return Tuple<Args&...>{ static_cast<Args&&>(args)... }; }

    template <typename TFunc, typename... Args>
    constexpr decltype(auto) TupleDispatch(TFunc&& function, Tuple<Args...>& tuple) noexcept
    {
        return[]<size_t... Is>(TFunc&& f, Tuple<Args...>& t) noexcept -> decltype(auto)
        {
            return PK::Forward<TFunc>(f)(TupleGetElement<Is>(t)...);
        }
        (PK::Forward<TFunc>(function), tuple);
    }
}
