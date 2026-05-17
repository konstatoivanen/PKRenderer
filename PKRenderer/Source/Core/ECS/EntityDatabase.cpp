#include "PrecompiledHeader.h"
#include "EntityDatabase.h"

namespace PK
{
    constexpr static uint32_t VIEW_BUCKET_COUNT_FACTOR = 3u;

    struct ViewNode
    {
        uint32_t id;
        int32_t previous;
        int32_t next;
        ViewNode() : id(0u), previous(-1), next(-1) {}
        ViewNode(uint32_t id) : id(id), previous(-1), next(-1) {}
    };

    // This could've been done in the header 
    // but I decided to try a pimpl implementation for fun instead of adhering to common architectural principles.
    // A copy of hashmap, but typeless.
    // Not calling destructors for views when destroyed.
    // We no longer care about reference tracking when this happens.
    struct ViewHeader
    {
        uint32_t viewSize = 0ll;
        uint32_t capacity = 0u;
        uint32_t bucketCount = 0u;
        uint32_t count = 0u;
        EntityViewDeleter deleter = nullptr;
        uint32_t* buckets = nullptr;
        ViewNode* nodes = nullptr;
        void* buffer = nullptr;
    };

    static ViewHeader* AllocateViewArray(EntityViewDeleter deleter, uint32_t viewSize, size_t capacity)
    {
        size_t size = sizeof(ViewHeader);
        auto offsetBuckets = (size + 15) & ~15;
        size = offsetBuckets + capacity * VIEW_BUCKET_COUNT_FACTOR * sizeof(uint32_t);
        auto offsetNodes = (size + 15) & ~15;
        size = offsetNodes + capacity * sizeof(ViewNode);
        auto offsetValues = (size + 15) & ~15;
        size = offsetValues + capacity * viewSize;
    
        auto memory = Memory::AllocateAligned(size);
        memset(memory, 0, size);

        auto header = reinterpret_cast<ViewHeader*>(memory);
        header->viewSize = viewSize;
        header->capacity = (uint32_t)capacity;
        header->bucketCount = (uint32_t)capacity * VIEW_BUCKET_COUNT_FACTOR;
        header->count = 0u;
        header->deleter = deleter;
        header->buckets = Memory::CastOffsetPtr<uint32_t>(memory, offsetBuckets);
        header->nodes = Memory::CastOffsetPtr<ViewNode>(memory, offsetNodes);
        header->buffer = Memory::CastOffsetPtr<void>(memory, offsetValues);
        return header;
    }



    void EntityDatabase::Delete(uint32_t typeIndex, uint32_t groupId, uint32_t entityId)
    {
        for (auto i = 0u; i < m_typedGroups.GetCount(); ++i)
        {
            if ((typeIndex == 0u || m_typedGroups[i].key.typeIndex() == typeIndex - 1u) || 
                (groupId == 0u || m_typedGroups[i].key.groupId() == groupId))
            {
                if (entityId == 0u)
                {
                    ClearViews(m_typedGroups[i].value);
                }
                else
                {
                    RemoveView(m_typedGroups[i].value, entityId);
                }
            }
        }
    }


    void* EntityDatabase::GetViewArrayData(EntityViewArray* views) { return views && views->header ? views->header->buffer : nullptr; }

    uint32_t EntityDatabase::GetViewArrayCount(const EntityViewArray* views) { return views && views->header ? views->header->count : 0u; }

    uint32_t EntityDatabase::GetViewIndex(const EntityViewArray* views, uint32_t id)
    {
        if (views && views->header && views->header->count)
        {
            const auto bucketIndex = id % views->header->bucketCount;
            auto valueIndex = (int32_t)(views->header->buckets[bucketIndex]) - 1;

            while (valueIndex != -1)
            {
                if (views->header->nodes[valueIndex].id == id)
                {
                    return valueIndex;
                }

                valueIndex = views->header->nodes[valueIndex].previous;
            }
        }

        return ~0u;
    }


    void EntityDatabase::CreateViewArray(EntityViewArray& viewArray, EntityViewDeleter deleter, uint32_t viewSize)
    {
        Memory::Assert(viewArray.header == nullptr, "View array is already initialized!");
        viewArray.header = AllocateViewArray(deleter, viewSize, 1u);
    }

    bool EntityDatabase::ReserveView(EntityViewArray& views, uint32_t id, void** outPtr)
    {
        const auto bucketIndex = id % views->bucketCount;
        const auto valueIndex = (int32_t)views->buckets[bucketIndex] - 1;
        auto movingValueIndex = valueIndex;

        while (movingValueIndex != -1)
        {
            if (views->nodes[movingValueIndex].id == id)
            {
                *outPtr = reinterpret_cast<uint8_t*>(views->buffer) + views->viewSize * movingValueIndex;
                return false;
            }

            movingValueIndex = views->nodes[movingValueIndex].previous;
        }

        const auto minCapacity = views->count + 1u;
        const auto resized = views->capacity < minCapacity;

        if (resized)
        {
            auto newViews = AllocateViewArray(views->deleter, views->viewSize, Hash::ExpandPrime(minCapacity));
            newViews->count = views->count;
            memcpy(newViews->buffer, views->buffer, views->viewSize * views->count);
            memcpy(newViews->nodes, views->nodes, sizeof(ViewNode) * views->count);
            Memory::Free(views.header);
            views.header = newViews;
        }

        const auto index = views->count++;
        views->nodes[index] = ViewNode(id);

        if (!resized)
        {
            views->nodes[index].previous = valueIndex;

            if (valueIndex != -1)
            {
                views->nodes[valueIndex].next = index;
            }

            views->buckets[bucketIndex] = index + 1u;
        }
        else
        {
            for (auto newValueIndex = 0u; newValueIndex < views->count; newValueIndex++)
            {
                const auto existingBucketIndex = views->nodes[newValueIndex].id % views->bucketCount;
                const auto existingValueIndex = (int32_t)views->buckets[existingBucketIndex] - 1;
                views->buckets[existingBucketIndex] = newValueIndex + 1u;

                if (existingValueIndex != -1)
                {
                    views->nodes[newValueIndex].previous = existingValueIndex;
                    views->nodes[newValueIndex].next = -1;
                    views->nodes[existingValueIndex].next = newValueIndex;
                }
                else
                {
                    views->nodes[newValueIndex].next = -1;
                    views->nodes[newValueIndex].previous = -1;
                }
            }
        }

        *outPtr = reinterpret_cast<uint8_t*>(views->buffer) + views->viewSize * index;
        return true;
    }
    
    void EntityDatabase::RemoveView(EntityViewArray& views, uint32_t id)
    {
        Memory::Assert(views.header, "Entity view array is not allocated!");

        const auto index = GetViewIndex(&views, id);

        if (index == ~0u)
        {
            return;
        }

        const auto bucketIndex = views->nodes[index].id % views->bucketCount;

        if (views->buckets[bucketIndex] == index + 1u)
        {
            views->buckets[bucketIndex] = (uint32_t)(views->nodes[index].previous + 1);
        }

        const auto updateNext = views->nodes[index].next;
        const auto updatePrevious = views->nodes[index].previous;

        if (updateNext != -1)
        {
            views->nodes[updateNext].previous = updatePrevious;
        }

        if (updatePrevious != -1)
        {
            views->nodes[updatePrevious].next = updateNext;
        }

        views->count--;
        views->deleter(reinterpret_cast<uint8_t*>(views->buffer) + views->viewSize * index);

        if (index != views->count)
        {
            const auto movingBucketIndex = views->nodes[views->count].id % views->bucketCount;

            if (views->buckets[movingBucketIndex] == views->count + 1u)
            {
                views->buckets[movingBucketIndex] = index + 1u;
            }

            const auto next = views->nodes[views->count].next;
            const auto previous = views->nodes[views->count].previous;

            if (next != -1)
            {
                views->nodes[next].previous = index;
            }

            if (previous != -1)
            {
                views->nodes[previous].next = index;
            }

            views->nodes[index] = views->nodes[views->count];
            auto dst = reinterpret_cast<uint8_t*>(views->buffer) + views->viewSize * index;
            auto src = reinterpret_cast<uint8_t*>(views->buffer) + views->viewSize * views->count;
            memcpy(dst, src, views->viewSize);
            memset(src, 0, views->viewSize);
        }
    }
    
    void EntityDatabase::ClearViews(EntityViewArray& views)
    {
        if (views.header && views->count)
        {
            for (auto i = 0u; i < views->count; ++i)
            {
                views->deleter(reinterpret_cast<uint8_t*>(views->buffer) + views->viewSize * i);
            }

            memset(views->buffer, 0, views->viewSize * views->count);
            Memory::ClearArray(views->nodes, views->count);
            Memory::Memset(views->buckets, 0, views->bucketCount);
            views->count = 0u;
        }
    }
}
