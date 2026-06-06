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
    
    typedef void (*EntityViewDeleter)(void*);

    struct alignas(16) ImplementerBucket
    {

        uint8_t data[PK_ECS_IMPLEMENTER_BUCKET_SIZE];
        void (*destroyAt)(ImplementerBucket* bucket, uint32_t index);
        ImplementerBucket* previous;
        uint64_t capacity;
        uint64_t freeCount;

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

    struct EntityViewArray
    {
        struct ViewHeader* header = nullptr;
        constexpr EntityViewArray() = default;
        EntityViewArray(EntityViewArray&& other) noexcept : header(PK::Exchange(other.header, nullptr)) {}
        ~EntityViewArray() { Memory::Free(header); header = nullptr; }
        EntityViewArray& operator=(EntityViewArray&& other) noexcept { header = PK::Exchange(other.header, nullptr); return *this; }
        ViewHeader* operator->() const noexcept { return header; }
    };

    struct EntityDatabase
    {
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
                CreateViewArray(m_typedGroups[groupIndex].value, [](void* memory) { Memory::Destruct(static_cast<TView*>(memory)); }, sizeof(TView));
            }

            if (ReserveView(m_typedGroups[groupIndex].value, egid.entityID(), &view))
            {
                Memory::Construct(static_cast<TView*>(view));
                static_cast<TView*>(view)->GID = egid;
            }

            return static_cast<TView*>(view);
        }

        template<typename TView, typename TImpl>
        TView* NewView(TImpl* implementer, const EGID& egid)
        {
            auto* view = NewView<TView>(egid);
            view->template SetImplementer<TImpl>(implementer);
            return view;
        }

        template<typename TView>
        const BufferView<TView> Query(const uint32_t groupId)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            const auto views = m_typedGroups.GetValuePtr(GroupKey(groupId, pk_base_type_index<TView>()));
            return { static_cast<TView*>(GetViewArrayData(views)), (size_t)GetViewArrayCount(views) };
        }

        template<typename TView>
        TView* Query(const EGID& egid)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            const auto views = m_typedGroups.GetValuePtr(GroupKey(egid.groupID(), pk_base_type_index<TView>()));
            const auto index = GetViewIndex(views, egid.entityID());
            return index != ~0u ? static_cast<TView*>(GetViewArrayData(views)) + index : nullptr;
        }

        template<typename TView>
        void DeleteAllOfType()
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            Delete(pk_base_type_index<TView>() + 1u, 0u, 0u);
        }
        
        template<typename TView>
        void DeleteGroupOfType(uint32_t groupId)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            Delete(pk_base_type_index<TView>() + 1u, groupId, 0u);
        }

        template<typename TView>
        void DeleteOfType(const EGID& egid)
        {
            static_assert(__is_base_of(IEntityView, TView), "Template argument type does not derive from IEntityView!");
            Delete(pk_base_type_index<TView>() + 1u, egid.groupID(), egid.entityID());
        }

        inline void DeleteGroup(uint32_t groupId) 
        { 
            Delete(0u, groupId, 0u); 
        }

        inline void Delete(const EGID& egid) 
        { 
            Delete(0u, egid.groupID(), egid.entityID()); 
        }

    private:
        void Delete(uint32_t typeIndex, uint32_t groupId, uint32_t entityId);

        static void* GetViewArrayData(EntityViewArray* views);
        static uint32_t GetViewArrayCount(const EntityViewArray* views);
        static uint32_t GetViewIndex(const EntityViewArray* views, uint32_t id);
        static void CreateViewArray(EntityViewArray& views, EntityViewDeleter deleter, uint32_t viewSize);
        static bool ReserveView(EntityViewArray& views, uint32_t id, void** outPtr);
        static void RemoveView(EntityViewArray& views, uint32_t id);
        static void ClearViews(EntityViewArray& views);

        HashMap<GroupKey, EntityViewArray, GroupHash> m_typedGroups;
        HashMap<uint32_t, ImplementerContainer> m_implementers;
        uint32_t m_idCounter = 0u;
    };
}
