#pragma once
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
        INACTIVE = 0,
        ACTIVE = 1,
        FREE = 2
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

        EntityViewsCollection(EntityViewsCollection&& other) noexcept : 
            indices(std::move(other.indices)), 
            buffer(std::move(other.buffer)), 
            head(std::exchange(other.head, 0ull))
        {
        }

        EntityViewsCollection& operator=(EntityViewsCollection&& other) noexcept
        {
            indices = std::move(other.indices);
            buffer = std::move(other.buffer);
            head = std::exchange(other.head, 0ull);
            return *this;
        }
    };

    //@TODO support removals
    struct EntityDatabase
    {
        EntityDatabase() : m_entityViews(32u, 4u), m_implementerBuckets(32u, 4u) {}

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

            const auto viewIdx = m_entityViews.AddKey(MakeViewKey<TView>(egid.groupID()));
            auto& views = m_entityViews[viewIdx].value;
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
            auto views = m_entityViews.GetValuePtr(MakeViewKey<TView>(group));
            auto data = views ? reinterpret_cast<TView*>(views->buffer.GetData()) : nullptr;
            auto count = views ? views->head / (sizeof(TView) / sizeof(uint64_t)) : 0ull;
            return { data, count };
        }

        template<typename TView>
        TView* Query(const EGID& egid)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");

            if (!egid.IsValid())
            {
                throw std::runtime_error("Trying to acquire resources for an invalid egid!");
            }

            auto views = m_entityViews.GetValuePtr(MakeViewKey<TView>(egid.groupID()));

            if (!views)
            {
                return nullptr;
            }

            auto offset = views->indices.GetValuePtr(egid.entityID());
            return offset ? reinterpret_cast<TView*>(views->buffer.GetData() + *offset) : nullptr;
        }

    private:
        template<typename TView>
        inline static uint64_t MakeViewKey(uint32_t group)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");
            return Hash::InterlaceHash32x2(group, pk_base_type_index<TView>());
        }

        FastMap<uint64_t, EntityViewsCollection, Hash::TCastHash<uint64_t>> m_entityViews;
        FastMap<uint32_t, ImplementerContainer, Hash::TCastHash<uint32_t>> m_implementerBuckets;
        uint32_t m_idCounter = 0u;
    };
}

