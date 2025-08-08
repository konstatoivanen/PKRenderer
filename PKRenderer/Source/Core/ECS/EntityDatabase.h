#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "Core/Utilities/BufferView.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/FastTypeIndex.h"
#include "Core/Utilities/MemoryBlock.h"
#include "Core/ECS/EGID.h"
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/IEntityImplementer.h"

namespace PK
{
    enum class ENTITY_GROUPS
    {
        INVALID = 0,
        INACTIVE = 1,
        ACTIVE = 2,
        FREE = 3
    };

    constexpr static const uint32_t PK_ECS_BUCKET_SIZE = 32000;

    // @TODO convert into a pool. for deletions.
    struct ImplementerBucket
    {
        Unique<ImplementerBucket> previous;
        size_t count;
        void (*destructor)(void* value, size_t count);
        uint8_t data[PK_ECS_BUCKET_SIZE];

        ~ImplementerBucket() { destructor(data, count); }
    };

    struct ImplementerContainer
    {
        size_t count = 0;
        Unique<ImplementerBucket> bucketHead;
    };

    struct EntityViewsCollection
    {
        FastMap<uint32_t, uint64_t, Hash::TCastHash<uint32_t>> indices;
        MemoryBlock<uint64_t> buffer;
        uint64_t head = 0ull;

        template<typename TView>
        static uint64_t MakeKey(uint32_t group)
        {
            return Hash::InterlaceHash32x2(group, pk_base_type_index<TView>());
        }
    };

    //@TODO support removals
    struct EntityDatabase
    {
        EntityDatabase() : m_implementerBuckets(32u, 4u) {}

        constexpr uint32_t ReserveEntityId() { return ++m_idCounter; }
        inline EGID ReserveEntityId(uint32_t groupId) { return EGID(ReserveEntityId(), groupId); }

        template<typename T>
        T* ReserveImplementer()
        {
            static_assert(std::is_base_of<IEntityImplementer, T>::value, "Template argument type does not derive from IImplementer!");

            const auto elementsPerBucket = PK_ECS_BUCKET_SIZE / sizeof(T);
            const auto containerIdx = m_implementerBuckets.AddKey(pk_base_type_index<T>());
            auto& container = m_implementerBuckets[containerIdx].value;

            if (!container.bucketHead || container.bucketHead->count >= elementsPerBucket)
            {
                auto bucket = CreateUnique<ImplementerBucket>();
                bucket->previous = std::move(container.bucketHead);
                bucket->count = 0u;
                bucket->destructor = [](void* data, size_t count)
                {
                    for (auto i = 0u; i < count; ++i)
                    {
                        (reinterpret_cast<T*>(data) + i)->~T();
                    }
                };
                
                container.bucketHead = std::move(bucket);
            }

            auto ptr = reinterpret_cast<T*>(container.bucketHead->data) + container.bucketHead->count;
            new(ptr) T();
            ++container.bucketHead->count;
            ++container.count;
            return ptr;
        }

        template<typename TView>
        TView* ReserveView(const EGID& egid)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!egid.IsValid())
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto& views = m_entityViews[EntityViewsCollection::MakeKey<TView>(egid.groupID())];
            auto viewSize = sizeof(TView) / sizeof(uint64_t);
            auto index = 0u;
            auto entityId = egid.entityID();

            if (views.indices.AddKey(entityId, &index))
            {
                auto head = views.head;
                auto count = 1u + head / viewSize;
                auto capacity = views.buffer.GetCount() / viewSize;
                views.buffer.Validate((size_t)Hash::ExpandSize(capacity, count) * viewSize);
                views.indices[index].value = head;
                views.head += viewSize;
                auto* element = reinterpret_cast<TView*>(views.buffer.GetData() + head);
                element->GID = egid;
                return element;
            }

            return reinterpret_cast<TView*>(views.buffer.GetData() + views.indices[index].value);
        }

        template<typename TImpl, typename TView, typename ...M>
        TView* ReserveView(TImpl* implementer, const EGID& egid, M TView::* ...params)
        {
            auto* view = ReserveView<TView>(egid);
            static_assert((... && std::is_assignable<decltype(view->*params), TImpl*>::value), "Components are not present in implementer");
            ((view->*params = static_cast<M>(implementer)), ...);
            return view;
        }

        template<typename TView>
        const BufferView<TView> Query(const uint32_t group)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!group)
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto& views = m_entityViews[EntityViewsCollection::MakeKey<TView>(group)];
            auto count = views.head / (sizeof(TView) / sizeof(uint64_t));
            return { reinterpret_cast<TView*>(views.buffer.GetData()), count };
        }

        template<typename TView>
        TView* Query(const EGID& egid)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!egid.IsValid())
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto& views = m_entityViews.at(EntityViewsCollection::MakeKey<TView>(egid.groupID()));
            auto offset = views.indices.GetValuePtr(egid.entityID());
            return offset ? reinterpret_cast<TView*>(views.buffer.GetData() + *offset) : nullptr;
        }

    private:
        std::unordered_map<uint64_t, EntityViewsCollection, Hash::TCastHash<uint64_t>> m_entityViews;
        FastMap<uint32_t, ImplementerContainer, Hash::TCastHash<uint32_t>> m_implementerBuckets;
        uint32_t m_idCounter = 0;
    };
}

