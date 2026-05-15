#include "PrecompiledHeader.h"
#include "EntityDatabase.h"

namespace PK
{
    void EntityDatabase::Delete(const EGID& egid)
    {
        Memory::Assert(egid.IsValid(), "Invalid Egid!");

        for (auto i = 0u; i < m_typedGroups.GetCount(); ++i)
        {
            if (m_typedGroups[i].key.groupId() == egid.groupID())
            {
                RemoveView(m_typedGroups[i].value, egid.entityID());
            }
        }
    }
    
    void EntityDatabase::Delete(uint32_t groupId)
    {
        Memory::Assert(groupId > 0, "Invalid Egid!");

        for (auto i = 0u; i < m_typedGroups.GetCount(); ++i)
        {
            if (m_typedGroups[i].key.groupId() == groupId)
            {
                ClearViews(m_typedGroups[i].value);
            }
        }
    }


    uint32_t EntityDatabase::GetViewIndex(const ViewArray* views, uint32_t id)
    {
        if (views && views->count > 0)
        {
            const auto bucketIndex = id % views->bucketCount;
            auto valueIndex = (int32_t)(views->buckets[bucketIndex]) - 1;

            while (valueIndex != -1)
            {
                if (views->nodes[valueIndex].id == id)
                {
                    return valueIndex;
                }

                valueIndex = views->nodes[valueIndex].previous;
            }
        }

        return ~0u;
    }

    bool EntityDatabase::ReserveView(ViewArray& views, uint32_t id, void** outPtr)
    {
        auto existingIndex = GetViewIndex(&views, id);

        if (existingIndex != ~0u)
        {
            *outPtr = reinterpret_cast<uint8_t*>(views.buffer) + views.viewSize * existingIndex;
            return false;
        }

        const auto minCapacity = views.count + 1u;
        const auto resized = views.capacity < minCapacity;

        if (resized)
        {
            views.capacity = views.capacity == 0 ? minCapacity : (uint32_t)Hash::ExpandPrime(minCapacity);
            views.bucketCount = views.capacity * VIEW_BUCKET_COUNT_FACTOR;

            size_t size = views.capacity * views.viewSize;
            const auto offsetBuckets = Memory::AlignSize<uint32_t>(size);
            size = offsetBuckets + sizeof(uint32_t) * (views.bucketCount);
            const auto offsetNode = Memory::AlignSize<ViewNode>(size);
            size = offsetNode + sizeof(ViewNode) * views.capacity;

            auto newBuffer = Memory::AllocateClear<uint8_t>(size);
            auto newBuckets = Memory::CastOffsetPtr<uint32_t>(newBuffer, offsetBuckets);
            auto newNodes = Memory::CastOffsetPtr<ViewNode>(newBuffer, offsetNode);

            if (views.buffer)
            {
                memcpy(newBuffer, views.buffer, views.viewSize * views.count);
                memcpy(newNodes, views.nodes, sizeof(ViewNode) * views.count);
                Memory::Free(views.buffer);
            }

            views.buffer = newBuffer;
            views.nodes = newNodes;
            views.buckets = newBuckets;
        }

        const auto index = views.count++;
        views.nodes[index] = ViewNode(id);

        if (!resized)
        {
            const auto bucketIndex = id % views.bucketCount;
            const auto valueIndex = (int32_t)views.buckets[bucketIndex] - 1;
            views.nodes[index].previous = valueIndex;

            if (valueIndex != -1)
            {
                views.nodes[valueIndex].next = index;
            }

            views.buckets[bucketIndex] = index + 1u;
        }
        else
        {
            for (auto newValueIndex = 0u; newValueIndex < views.count; newValueIndex++)
            {
                const auto existingBucketIndex = views.nodes[newValueIndex].id % views.bucketCount;
                const auto existingValueIndex = (int32_t)views.buckets[existingBucketIndex] - 1;
                views.buckets[existingBucketIndex] = newValueIndex + 1u;

                if (existingValueIndex != -1)
                {
                    views.nodes[newValueIndex].previous = existingValueIndex;
                    views.nodes[newValueIndex].next = -1;
                    views.nodes[existingValueIndex].next = newValueIndex;
                }
                else
                {
                    views.nodes[newValueIndex].next = -1;
                    views.nodes[newValueIndex].previous = -1;
                }
            }
        }

        *outPtr = reinterpret_cast<uint8_t*>(views.buffer) + views.viewSize * index;
        return true;
    }
    
    void EntityDatabase::RemoveView(ViewArray& views, uint32_t id)
    {
        Memory::Assert(views.viewSize && views.destroyView, "Entity view array is badly formed!");

        const auto index = GetViewIndex(&views, id);

        if (index == ~0u)
        {
            return;
        }

        const auto bucketIndex = views.nodes[index].id % views.bucketCount;

        if (views.buckets[bucketIndex] == index + 1u)
        {
            views.buckets[bucketIndex] = (uint32_t)(views.nodes[index].previous + 1);
        }

        const auto updateNext = views.nodes[index].next;
        const auto updatePrevious = views.nodes[index].previous;

        if (updateNext != -1)
        {
            views.nodes[updateNext].previous = updatePrevious;
        }

        if (updatePrevious != -1)
        {
            views.nodes[updatePrevious].next = updateNext;
        }

        views.count--;
        views.destroyView(reinterpret_cast<uint8_t*>(views.buffer) + views.viewSize * index);

        if (index != views.count)
        {
            const auto movingBucketIndex = views.nodes[views.count].id % views.bucketCount;

            if (views.buckets[movingBucketIndex] == views.count + 1u)
            {
                views.buckets[movingBucketIndex] = index + 1u;
            }

            const auto next = views.nodes[views.count].next;
            const auto previous = views.nodes[views.count].previous;

            if (next != -1)
            {
                views.nodes[next].previous = index;
            }

            if (previous != -1)
            {
                views.nodes[previous].next = index;
            }

            views.nodes[index] = views.nodes[views.count];
            auto dst = reinterpret_cast<uint8_t*>(views.buffer) + views.viewSize * index;
            auto src = reinterpret_cast<uint8_t*>(views.buffer) + views.viewSize * views.count;
            memcpy(dst, src, views.viewSize);
            memset(src, 0, views.viewSize);
        }
    }
    
    void EntityDatabase::ClearViews(ViewArray& views)
    {
        if (views.count)
        {
            Memory::Assert(views.viewSize && views.destroyView, "Entity view array is badly formed!");

            for (auto i = 0u; i < views.count; ++i)
            {
                views.destroyView(reinterpret_cast<uint8_t*>(views.buffer) + views.viewSize * i);
            }

            memset(views.buffer, 0, views.viewSize * views.count);
            Memory::ClearArray(views.nodes, views.count);
            Memory::Memset(views.buckets, 0, views.bucketCount);
            views.count = 0u;
        }
    }
}
