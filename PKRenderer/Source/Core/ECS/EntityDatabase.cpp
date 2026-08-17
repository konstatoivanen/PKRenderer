#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "EntityDatabase.h"

namespace PK
{
    EntityDatabase::EntityDatabase(size_t compositionCapacity, size_t entityCapacity) :
        m_identifiers(entityCapacity, 1u),
        m_compositions(compositionCapacity, 1u)
    {
    }

    EntityDatabase::~EntityDatabase()
    {
        for (auto i = 0u; i < m_compositions.GetCount(); ++i)
        {
            Memory::Free(m_compositions[i].value.buffer);
        }
    }

    EntityDatabase::Identifier EntityDatabase::NewEntity(uint32_t entityIndex)
    {
        auto* comp = &m_compositions[entityIndex].value;
        auto arrayIndex = comp->count++;
        auto entityId = ++m_idCounter;
        auto identifier = Identifier(entityId, entityIndex, arrayIndex);

        for (auto i = 0u; i < comp->componentCount; ++i)
        {
            comp->components[i].constructAt(comp->streams[i], arrayIndex);
        }

        GetEntityIdStream(entityIndex)[arrayIndex] = entityId;
        m_identifiers.Add(identifier);
        return identifier;
    }

    void EntityDatabase::Delete(uint32_t entityId)
    {
        auto identifierIndex = m_identifiers.GetIndex(Identifier(entityId, 0u, 0u));

        if (identifierIndex != -1)
        {
            const auto identifier = m_identifiers[identifierIndex];
            const auto entityIndex = identifier.entityIndex();
            const auto arrayIndex = identifier.arrayIndex();
            m_identifiers.RemoveAt(identifierIndex);

            auto* comp = &m_compositions[entityIndex].value;
            comp->count--;

            for (auto i = 0u; i < comp->componentCount; ++i)
            {
                comp->components[i].removeAt(comp->streams[i], identifier.arrayIndex(), comp->count);
            }

            // Remove at swaps entity positions in the component streams.
            // Update array index in identifiers to match new array position.
            const auto swapEntityId = GetEntityIdStream(entityIndex)[arrayIndex];
            const auto swapIndex = m_identifiers.GetIndex(Identifier(swapEntityId, 0u, 0u));
            m_identifiers[swapIndex] = Identifier(swapEntityId, entityIndex, arrayIndex);
        }
    }

    void EntityDatabase::DeleteType(uint32_t typeIndex)
    {
        const auto typeKey = typeIndex & 0x7FFFFFFFu;
        const auto entityIndex = m_compositions.GetIndex(typeKey);

        if (entityIndex != -1)
        {
            auto* comp = &m_compositions[entityIndex].value;
            auto entityIdStream = GetEntityIdStream(entityIndex);

            // Better to do this in reverse order so that identifiers removal is more likely to hit a fast clear.
            for (int32_t i = comp->count - 1; i >= 0; --i)
            {
                m_identifiers.Remove(Identifier(entityIdStream[i], 0u, 0u));
            }
            
            for (auto i = 0u; i < comp->componentCount; ++i)
            {
                comp->components[i].clear(comp->streams[i], comp->count);
            }

            comp->count = 0u;
        }
    }


    uint32_t* EntityDatabase::GetEntityIdStream(uint32_t entityIndex)
    {
        auto* comp = &m_compositions[entityIndex].value;
        constexpr auto entityIdUUID = pk_ecs_type_uuid<uint32_t>;

        for (auto i = 0u; i < comp->componentCount; ++i)
        {
            if (comp->components[i].typeUUID == entityIdUUID)
            {
                return static_cast<uint32_t*>(comp->streams[i]);
            }
        }

        return nullptr;
    }

    void EntityDatabase::ReserveEntitities(uint32_t entityIndex, size_t entryCount)
    {
        auto* comp = &m_compositions[entityIndex].value;
        const auto isEntity = (m_compositions[entityIndex].key & 0x80000000u) == 0u;
        const auto isNew = comp->buffer == nullptr;
        const auto isPrimeExpand = !isNew && entryCount == 1ull;

        PK_DEBUG_WARNING_ASSERT(isEntity, "Trying to reserve entities for a view composition!");

        if (comp->capacity - comp->count >= entryCount)
        {
            return;
        }

        // Expand using primes if this is an allocate call for a single entity.
        // Allow custom increments for preallocation calls.
        auto newCapacity = 0u;
        newCapacity += comp->count;
        newCapacity += entryCount;
        newCapacity = isPrimeExpand ? Hash::ExpandPrime(newCapacity) : newCapacity;

        // Allocate some padding so that we can align all buffers to 16 byte boundaries.
        auto bufferSize = 0ull;
        bufferSize += sizeof(void*) * comp->componentCount;
        bufferSize += 16ull * (comp->componentCount + 1ull);
        bufferSize += comp->componentStride * newCapacity;

        auto offset = 0ull;
        auto buffer = Memory::AllocateClear<uint8_t>(bufferSize);

        auto streams = reinterpret_cast<void**>(buffer);
        offset += sizeof(void*) * comp->componentCount;
        offset = (offset + 15ull) & ~(15ull);

        for (auto i = 0u; i < comp->componentCount; ++i)
        {
            streams[i] = buffer + offset;
            offset += newCapacity * comp->components[i].stride;
            offset = (offset + 15ull) & ~(15ull);
            
            if (!isNew)
            {
                comp->components[i].move(streams[i], comp->streams[i], comp->count);
            }
        }

        Memory::Free(comp->buffer);
        comp->buffer = buffer;
        comp->streams = streams;
        comp->capacity = newCapacity;

        // Very inefficient double loop to update view indices
        // But this only happens once per entity so whatever.
        for (auto i = 0u; i < m_compositions.GetCount() && isNew; ++i)
        {
            if (m_compositions[i].key & 0x80000000u)
            {
                UpdateViewIndices(i);
            }
        }
    }

    void EntityDatabase::UpdateViewIndices(uint32_t viewIndex)
    {
        auto* view = &m_compositions[viewIndex].value;
        auto indices = static_cast<uint32_t*>(view->buffer);
        auto newCount = 0u;
        auto historyOffset = 0u;
        auto historyCount = 0u;

        for (auto i = 0u; i < m_compositions.GetCount(); ++i)
        {
            if (m_compositions[i].key & 0x80000000u)
            {
                continue;
            }

            auto* comp = &m_compositions[i].value;
            auto remainingMatches = view->componentCount;

            for (auto j = 0u; j < view->componentCount; ++j)
            for (auto k = 0u; k < comp->componentCount; ++k)
            {
                remainingMatches -= view->components[j].typeUUID == comp->components[k].typeUUID;
            }

            if (!remainingMatches)
            {
                if (historyCount < view->capacity)
                {
                    indices[historyCount++] = i;
                    historyOffset = i;
                }
    
                newCount++;
            }
        }

        if (view->capacity < newCount)
        {
            auto newIndices = Memory::AllocateClear<uint32_t>(newCount);

            if (historyCount)
            {
                Memory::CopyArray(newIndices, indices, historyCount);
            }

            for (auto i = historyOffset; i < m_compositions.GetCount(); ++i)
            {
                if (m_compositions[i].key & 0x80000000u)
                {
                    continue;
                }

                auto* comp = &m_compositions[i].value;
                auto remainingMatches = view->componentCount;

                for (auto j = 0u; j < view->componentCount; ++j)
                for (auto k = 0u; k < comp->componentCount; ++k)
                {
                    remainingMatches -= view->components[j].typeUUID == comp->components[k].typeUUID;
                }

                if (!remainingMatches)
                {
                    newIndices[historyCount++] = i;
                }
            }

            Memory::Free(view->buffer);
            view->buffer = newIndices;
            view->capacity = newCount;
        }

        view->count = newCount;
    }
}
