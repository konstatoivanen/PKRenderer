#pragma once
#include "Core/ECS/EntityComposition.h"

namespace PK
{
    struct EntityDatabase;

    template <typename T> 
    struct TEntityVisitorTraits
    { 
        static constexpr bool IsValid = false; 
    };

    template <typename U> 
    struct TEntityVisitorTraits<void(*)(EntityDatabase*, uint32_t*, U*)>
    {
        static constexpr bool IsValid = true;
        using Type = U;
    };

    struct EntityVisitor
    {
        const uint64_t uuid;
        void (*const visit)(EntityDatabase* db, uint32_t* entityId, void* userdata);

        template <auto TFunc>
        requires TEntityVisitorTraits<decltype(TFunc)>::IsValid
        constexpr static EntityVisitor Get() noexcept 
        { 
            using T = typename TEntityVisitorTraits<decltype(TFunc)>::Type;
            
            auto invoker = [](EntityDatabase* db, uint32_t* entityId, void* userdata) 
            {
                TFunc(db, entityId, static_cast<T*>(userdata));
            };

            return { pk_type_uuid64<T>, invoker }; 
        }
    };

    template <size_t N>
    struct EntityVisitorArray
    {
        EntityVisitor data[N];
    };

    struct EntityVisitorsView
    {
        const EntityVisitor* visitors;
        uint64_t count;
    };

    template<auto... TFuncs>
    inline constexpr auto entity_visitors = EntityVisitorArray<sizeof...(TFuncs)>{ { EntityVisitor::Get<TFuncs>()... } };

    template <auto... TFuncs>
    inline consteval EntityVisitorsView MakeEntityVisitorsView() noexcept
    {
        return EntityVisitorsView { entity_visitors<TFuncs...>.data, sizeof...(TFuncs) };
    }

    template <typename TEntity>
    concept TEntityHasVisitors = requires
    {
        TEntity::GetVisitors();
        requires TIsSame<decltype(TEntity::GetVisitors()), EntityVisitorsView>;
    };

    template<typename TEntity>
    requires TEntityHasVisitors<TEntity>
    inline constexpr EntityVisitorsView GetEntityVisitors() noexcept
    {
        return TEntity::GetVisitors();
    }

    template<typename TEntity>
    requires (!TEntityHasVisitors<TEntity>)
    inline constexpr EntityVisitorsView GetEntityVisitors() noexcept
    {
        return EntityVisitorsView{ nullptr, 0ull };
    }
}
