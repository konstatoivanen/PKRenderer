#pragma once
#include "Core/Utilities/BufferView.h"
#include "Core/Utilities/HashMap.h"
#include "Core/Utilities/TypeIndex.h"
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

    constexpr static const uint32_t PK_ECS_IMPLEMENTER_BUCKET_SIZE = 32768u;
    
    struct alignas(16) ImplementerBucket
    {

        uint8_t data[PK_ECS_IMPLEMENTER_BUCKET_SIZE];
        void (*destroyAt)(ImplementerBucket* bucket, uint32_t index);
        ImplementerBucket* previous;
        uint32_t capacity;
        uint32_t freeCount;

        ~ImplementerBucket() 
        { 
            Memory::Delete(previous);

            for (auto i = 0u; i < capacity; ++i)
            {
                destroyAt(this, i);
            }
        }
    };

    struct ImplementerContainer
    {
        ImplementerBucket* bucketHead;
        ~ImplementerContainer() { Memory::Delete(bucketHead); }
    };

    //@TODO support group switches
    struct EntityDatabase
    {
        constexpr static uint32_t VIEW_BUCKET_COUNT_FACTOR = 3u;

        struct GroupKey
        {
            uint64_t identifier = 0ull;
            GroupKey(uint32_t groupId, const uint32_t typeIndex) : identifier((uint64_t)typeIndex | ((uint64_t)groupId << 32ull)) {}
            uint32_t typeIndex() const { return identifier & 0xFFFFFFFFull; }
            uint32_t groupId() const { return (identifier >> 32ull) & 0xFFFFFFFFull; }
            bool operator == (const GroupKey& other) { return identifier == other.identifier; }
            bool operator != (const GroupKey& other) { return identifier != other.identifier; }
        };

        struct GroupHash
        {
            size_t operator()(const GroupKey& k) const noexcept
            {
                return (size_t)k.identifier;
            }
        };

        struct ViewNode
        {
            uint32_t id;
            int32_t previous;
            int32_t next;
            ViewNode() : id(0u), previous(-1), next(-1) {}
            ViewNode(uint32_t id) : id(id), previous(-1), next(-1) {}
        };

        // A copy of hashmap, but typeless.
        // Not calling destructors for views when destroyed.
        // We no longer care about reference tracking when this happens.
        struct ViewArray
        {
            constexpr ViewArray() = default;

            ViewArray(ViewArray&& other) noexcept :
                buffer(PK::Exchange(other.buffer, nullptr)),
                buckets(PK::Exchange(other.buckets, nullptr)),
                nodes(PK::Exchange(other.nodes, nullptr)),
                destroyView(PK::Exchange(other.destroyView, nullptr)),
                bucketCount(PK::Exchange(other.bucketCount, 0u)),
                viewSize(PK::Exchange(other.viewSize, 0u)),
                capacity(PK::Exchange(other.capacity, 0u)),
                count(PK::Exchange(other.count, 0u))
            { 
            }

            ~ViewArray() 
            { 
                Memory::Free(buffer); 
                buffer = nullptr; 
            }

            inline ViewArray& operator=(ViewArray&& other) noexcept 
            { 
                if (this != &other)
                {
                    if (buffer)
                    {
                        Memory::Free(buffer);
                    }

                    buffer = PK::Exchange(other.buffer, nullptr);
                    buckets = PK::Exchange(other.buckets, nullptr);
                    nodes = PK::Exchange(other.nodes, nullptr);
                    destroyView = PK::Exchange(other.destroyView, nullptr);
                    bucketCount = PK::Exchange(other.bucketCount, 0u);
                    viewSize = PK::Exchange(other.viewSize, 0u);
                    capacity = PK::Exchange(other.capacity, 0u);
                    count = PK::Exchange(other.count, 0u);
                }

                return *this;
            }

            void* buffer = nullptr;
            uint32_t* buckets = nullptr;
            ViewNode* nodes = nullptr;
            void (*destroyView)(void*) = nullptr;
            uint32_t bucketCount = 0u;
            uint32_t viewSize = 0ll;
            uint32_t capacity = 0u;
            uint32_t count = 0u;
        };

        EntityDatabase() : m_typedGroups(32u, 3u), m_implementers(32u, 3u) {}

        constexpr uint32_t ReserveEntityId() { return ++m_idCounter; }
        inline EGID ReserveEntityId(uint32_t groupId) { return EGID(ReserveEntityId(), groupId); }

        template<typename T>
        T* NewImplementer()
        {
            static_assert(__is_base_of(IEntityImplementer, T), "Template argument type does not derive from IImplementer!");

            const auto elementsPerBucket = PK_ECS_IMPLEMENTER_BUCKET_SIZE / sizeof(T);
            const auto containerIndex = m_implementers.AddKey(pk_base_type_index<T>());
            auto& container = m_implementers[containerIndex].value;
            auto bucket = container.bucketHead;

            // Find bucket with free slots.
            for (; bucket && !bucket->freeCount; bucket = bucket->previous) {}

            // Nothing available. create a new bucket.
            if (!bucket || !bucket->freeCount)
            {
                bucket = Memory::New<ImplementerBucket>();
                bucket->previous = PK::Exchange(container.bucketHead, bucket);
                bucket->capacity = elementsPerBucket;
                bucket->freeCount = elementsPerBucket;
                bucket->destroyAt = [](ImplementerBucket* bucket, uint32_t index)
                {
                    auto element = static_cast<T*>((void*)bucket->data) + index;

                    if (element->bucket)
                    {
                        element->bucket = nullptr;
                        Memory::Destruct(element);
                    }
                };
            }

            const auto beg = static_cast<T*>((void*)bucket->data);
            const auto end = beg + elementsPerBucket;
            auto ptr = beg;

            for (; ptr->bucket && ptr < end; ++ptr) {}

            bucket->freeCount--;
            Memory::Construct(ptr);
            ptr->referenceCount = 0u;
            ptr->index = (uint32_t)(ptr - beg);
            ptr->bucket = bucket;
            return ptr;
        }

        template<typename TView>
        TView* NewView(const EGID& egid)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            Memory::Assert(egid.IsValid(), "Invalid Egid!");

            uint32_t groupIndex = 0u;
            void* view = nullptr;
            
            if (m_typedGroups.AddKey(GroupKey(egid.groupID(), pk_base_type_index<TView>()), &groupIndex))
            {
                m_typedGroups[groupIndex].value.viewSize = sizeof(TView);
                m_typedGroups[groupIndex].value.destroyView = [](void* memory) 
                { 
                    Memory::Destruct(static_cast<TView*>(memory)); 
                };
            }

            if (ReserveView(m_typedGroups[groupIndex].value, egid.entityID(), &view))
            {
                Memory::Construct(static_cast<TView*>(view));
                static_cast<TView*>(view)->GID = egid;
            }

            return static_cast<TView*>(view);
        }

        template<typename TImpl, typename TView, typename ...M>
        TView* NewView(TImpl* implementer, const EGID& egid, M TView::* ...params)
        {
            auto* view = NewView<TView>(egid);
            static_assert((... && __is_assignable(decltype(view->*params), TImpl*)), "Components are not present in implementer");
            ((view->*params = implementer), ...);
            return view;
        }

        template<typename TView>
        const BufferView<TView> Query(const uint32_t groupId)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            const auto views = m_typedGroups.GetValuePtr(GroupKey(groupId, pk_base_type_index<TView>()));
            return { views ? static_cast<TView*>(views->buffer) : nullptr, views ? views->count : 0ull };
        }

        template<typename TView>
        TView* Query(const EGID& egid)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            const auto views = m_typedGroups.GetValuePtr(GroupKey(egid.groupID(), pk_base_type_index<TView>()));
            const auto index = GetViewIndex(views, egid.entityID());
            return index != ~0u ? static_cast<TView*>(views->buffer) + index : nullptr;
        }

        void Delete(const EGID& egid);
        void Delete(uint32_t group);

    private:
        static uint32_t GetViewIndex(const ViewArray* views, uint32_t id);
        static bool ReserveView(ViewArray& views, uint32_t id, void** outPtr);
        static void RemoveView(ViewArray& views, uint32_t id);
        static void ClearViews(ViewArray& views);

        HashMap<GroupKey, ViewArray, GroupHash> m_typedGroups;
        HashMap<uint32_t, ImplementerContainer> m_implementers;
        uint32_t m_idCounter = 0u;
    };
}
