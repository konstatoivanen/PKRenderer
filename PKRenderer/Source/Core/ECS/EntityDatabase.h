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

    struct EntityViewContainer
    {
        MemoryBlock<uint64_t> buffer;
        uint64_t head = 0ull;

        EntityViewContainer(EntityViewContainer&& other) noexcept :
            buffer(std::move(other.buffer)), 
            head(std::exchange(other.head, 0ull))
        {
        }

        EntityViewContainer& operator=(EntityViewContainer&& other) noexcept
        {
            buffer = std::move(other.buffer);
            head = std::exchange(other.head, 0ull);
            return *this;
        }
    };

    struct EntityViewHeader
    {
        uint64_t identifier = 0ull;
        uint32_t container = 0u;
        uint32_t offset = 0u;

        EntityViewHeader(uint32_t groupId, const uint32_t typeIndex)
        {
            identifier |= (typeIndex & 0xFFFFFFull);
            identifier |= (groupId & 0xFFFFull) << 24ull;
        }

        EntityViewHeader(const EGID& egid, const uint32_t typeIndex)
        {
            //@TODO Swizzle me
            identifier |= (typeIndex & 0xFFFFFFull);
            identifier |= (egid.groupID() & 0xFFFFull) << 24u;
            identifier |= (egid.entityID() & 0xFFFFFFull) << 40ull;
        }

        uint32_t typeIndex() const { return identifier & 0xFFFFFFull; }
        uint32_t groupId() const { return (identifier >> 24ull) & 0xFFFFull; }
        uint32_t entityId() const { return (identifier >> 40ull) & 0xFFFFFFull; }

        bool operator == (const EntityViewHeader& other) { return identifier == other.identifier; }
        bool operator != (const EntityViewHeader& other) { return identifier != other.identifier; }
    };

    struct EntityViewHeaderHash
    {
        size_t operator()(const EntityViewHeader& k) const noexcept
        {
            return (size_t)k.identifier;
        }
    };

    //@TODO support removals & group switches
    struct EntityDatabase
    {
        EntityDatabase() : m_viewHeaders(1024u, 5u), m_entityViews(32), m_implementers(32u, 4u) {}

        ~EntityDatabase()
        {
            EntityViewContainer* containters = m_entityViews.GetData();

            for (auto i = 0u; i < m_viewCounter; ++i)
            {
                (containters + i)->~EntityViewContainer();
            }
        }

        constexpr uint32_t ReserveEntityId() { return ++m_idCounter; }
        inline EGID ReserveEntityId(uint32_t groupId) { return EGID(ReserveEntityId(), groupId); }

        template<typename T>
        T* ReserveImplementer()
        {
            static_assert(std::is_base_of<IEntityImplementer, T>::value, "Template argument type does not derive from IImplementer!");

            const auto elementsPerBucket = PK_ECS_BUCKET_SIZE / sizeof(T);
            const auto containerIdx = m_implementers.AddKey(pk_base_type_index<T>());
            auto& container = m_implementers[containerIdx].value;

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

            const uint32_t groupIdx = m_viewHeaders.Add(EntityViewHeader(egid.groupID(), pk_base_type_index<TView>()));
            auto& group = m_viewHeaders[groupIdx];

            if (group.container == 0u)
            {
                m_entityViews.Validate(++m_viewCounter);
                group.container = m_viewCounter;
            }

            auto& views = m_entityViews[group.container - 1u];
            const auto viewIdx = m_viewHeaders.Add(EntityViewHeader(egid, pk_base_type_index<TView>()));
            auto& view = m_viewHeaders[viewIdx];

            if (view.container == 0u)
            {
                auto viewSize = sizeof(TView) / sizeof(uint64_t);
                auto head = views.head;
                auto count = 1u + head / viewSize;
                auto capacity = views.buffer.GetCount() / viewSize;
                views.buffer.Validate((size_t)Hash::ExpandSize(capacity, count) * viewSize);
                view.container = group.container;
                view.offset = head;
                views.head += viewSize;
                reinterpret_cast<TView*>(views.buffer.GetData() + view.offset)->GID = egid;
            }

            return reinterpret_cast<TView*>(views.buffer.GetData() + view.offset);
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
            const auto* header = m_viewHeaders.GetValuePtr(EntityViewHeader(group, pk_base_type_index<TView>()));
            auto data = header ? reinterpret_cast<TView*>(m_entityViews[header->container - 1u].buffer.GetData()) : nullptr;
            auto count = header ? m_entityViews[header->container - 1u].head / (sizeof(TView) / sizeof(uint64_t)) : 0ull;
            return { data, count };
        }

        template<typename TView>
        TView* Query(const EGID& egid)
        {
            static_assert(std::is_base_of<IEntityView, TView>::value, "Template argument type does not derive from IEntityView!");
            const auto* header = m_viewHeaders.GetValuePtr(EntityViewHeader(egid, pk_base_type_index<TView>()));
            return header ? reinterpret_cast<TView*>(m_entityViews[header->container - 1u].buffer.GetData() + header->offset) : nullptr;
        }

    private:
        FastSet<EntityViewHeader, EntityViewHeaderHash> m_viewHeaders;
        MemoryBlock<EntityViewContainer> m_entityViews;
        FastMap<uint32_t, ImplementerContainer, Hash::TCastHash<uint32_t>> m_implementers;
        uint32_t m_idCounter = 0u;
        uint32_t m_viewCounter = 0u;
    };
}
