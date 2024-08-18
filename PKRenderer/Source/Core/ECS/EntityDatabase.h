#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "Core/Utilities/BufferView.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FastMap.h"
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
        Scope<ImplementerBucket> previous;
        size_t count;
        void (*destructor)(void* value, size_t count);
        uint8_t data[PK_ECS_BUCKET_SIZE];

        ~ImplementerBucket() { destructor(data, count); }
    };

    struct ImplementerContainer
    {
        size_t count = 0;
        Scope<ImplementerBucket> bucketHead;
    };

    struct EntityViewsCollection
    {
        FastMap<uint32_t, uint64_t, Hash::TCastHash<uint32_t>> indices;
        MemoryBlock<uint64_t> buffer;
        uint64_t head = 0ull;
    };

    struct ViewCollectionKey
    {
        std::type_index type;
        uint32_t group;

        inline bool operator == (const ViewCollectionKey& r) const noexcept
        {
            return type == r.type && group == r.group;
        }
    };

    struct ViewCollectionKeyHash 
    {
        std::size_t operator()(const ViewCollectionKey& k) const
        {
            return k.type.hash_code() ^ (k.group * 1099511628211ULL);
        }
    };

    //@TODO support removals
    struct EntityDatabase
    {
        constexpr uint32_t ReserveEntityId() { return ++m_idCounter; }
        inline EGID ReserveEntityId(uint32_t groupId) { return EGID(ReserveEntityId(), groupId); }

        template<typename T>
        T* ReserveImplementer()
        {
            static_assert(std::is_base_of<IEntityImplementer, T>::value, "Template argument type does not derive from IImplementer!");

            auto type = std::type_index(typeid(T));
            auto elementsPerBucket = PK_ECS_BUCKET_SIZE / sizeof(T);
            auto& container = m_implementerBuckets[type];

            if (!container.bucketHead || container.bucketHead->count >= elementsPerBucket)
            {
                auto bucket = CreateScope<ImplementerBucket>();
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

            auto& views = m_entityViews[{ std::type_index(typeid(TView)), egid.groupID() }];
            auto viewSize = sizeof(TView) / sizeof(uint64_t);
            auto index = 0u;

            if (views.indices.AddKey(egid.entityID(), &index))
            {
                auto head = views.head;
                auto count = 1u + head / viewSize;
                auto capacity = views.buffer.GetCount() / viewSize;
                views.buffer.Validate((size_t)Hash::ExpandSize(capacity, count) * viewSize);
                views.indices.SetValueAt(index, head);
                views.head += viewSize;
                auto* element = reinterpret_cast<TView*>(views.buffer.GetData() + head);
                element->GID = egid;
                return element;
            }

            return reinterpret_cast<TView*>(views.buffer.GetData() + views.indices.GetValueAt(index));
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

            auto& views = m_entityViews[{ std::type_index(typeid(TView)), group }];
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

            auto& views = m_entityViews.at({ std::type_index(typeid(TView)), egid.groupID() });
            auto offset = views.indices.GetValueRef(egid.entityID());
            return offset ? reinterpret_cast<TView*>(views.buffer.GetData() + *offset) : nullptr;
        }

    private:
        std::unordered_map<ViewCollectionKey, EntityViewsCollection, ViewCollectionKeyHash> m_entityViews;
        std::unordered_map<std::type_index, ImplementerContainer> m_implementerBuckets;
        uint32_t m_idCounter = 0;
    };
}

