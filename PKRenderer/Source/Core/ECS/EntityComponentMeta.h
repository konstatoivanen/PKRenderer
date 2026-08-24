#pragma once
#include "Core/ECS/EntityComposition.h"

namespace PK
{
    struct EntityComponentMeta
    {
        const uint64_t typeUUID;
        const uint64_t stride;
        void (*const constructAt)(void* data, uint32_t index);
        void (*const removeAt)(void* data, uint32_t index, uint32_t last);
        void (*const clear)(void* data, uint32_t count);
        void (*const move)(void* dst, void* src, uint32_t count);

        template<typename T>
        constexpr static EntityComponentMeta Get() noexcept
        {
            return
            {
                pk_type_uuid64<T>,
                sizeof(T),
                [](void* data, uint32_t index) { Memory::Construct(static_cast<T*>(data) + index);},
                [](void* data, uint32_t index, uint32_t last) { static_cast<T*>(data)[index] = PK::MoveTemp(static_cast<T*>(data)[last]); },
                [](void* data, uint32_t count) { Memory::ClearArray(static_cast<T*>(data), count); },
                [](void* dst, void* src, uint32_t count) { Memory::MoveArray(static_cast<T*>(dst), static_cast<T*>(src), count); }
            };
        }
    };

    template <size_t N>
    struct EntityComponentMetaArray
    {
        EntityComponentMeta data[N];
    };

    template<typename T>
    inline constexpr auto entity_component_metas = []<size_t... I>(TIndexSequence<I...>)
    {
        return EntityComponentMetaArray<T::Size>{{ EntityComponentMeta::Get<typename Sequence::TypeAt<I, T>>()... }};
    }
    (TMakeIndexSequence<T::Size>{});
}
