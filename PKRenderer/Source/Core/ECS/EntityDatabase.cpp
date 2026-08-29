#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "EntityDatabase.h"

namespace PK
{
    EntityDatabase::EntityDatabase(uint32_t compositionCapacity, uint32_t entityCapacity) :
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

    EntityID EntityDatabase::NewEntity(uint32_t compositionIndex)
    {
        auto* comp = &m_compositions[compositionIndex].value;
        auto arrayIndex = comp->count++;
        auto entityId = ++m_idCounter;
        auto identifier = EntityID(entityId, compositionIndex, arrayIndex);

        for (auto i = 0u; i < comp->componentCount; ++i)
        {
            comp->components[i].constructAt(comp->GetStreams()[i], arrayIndex);
        }

        GetEntityIdStream(compositionIndex)[arrayIndex] = entityId;
        m_identifiers.Add(identifier);
        return identifier;
    }

    void EntityDatabase::Delete(uint32_t entityId)
    {
        auto identifierIndex = m_identifiers.GetHashIndex(entityId);

        if (identifierIndex != -1)
        {
            const auto identifier = m_identifiers[identifierIndex];
            const auto compositionIndex = identifier.compositionIndex();
            const auto arrayIndex = identifier.arrayIndex();
            m_identifiers.RemoveAt(identifierIndex);

            auto* comp = &m_compositions[compositionIndex].value;
            comp->count--;

            for (auto i = 0u; i < comp->componentCount; ++i)
            {
                comp->components[i].removeAt(comp->GetStreams()[i], identifier.arrayIndex(), comp->count);
            }

            // Remove at swaps entity positions in the component streams.
            // Update array index in identifiers to match new array position.
            const auto swapEntityId = GetEntityIdStream(compositionIndex)[arrayIndex];
            const auto swapIndex = m_identifiers.GetHashIndex(swapEntityId);
            m_identifiers[swapIndex] = EntityID(swapEntityId, compositionIndex, arrayIndex);
        }
    }

    void EntityDatabase::DeleteType(uint64_t compositionUUID)
    {
        const auto typeKey = compositionUUID & COMP_MASK;
        const auto compositionIndex = m_compositions.GetIndex(typeKey);

        if (compositionIndex != -1)
        {
            auto* comp = &m_compositions[compositionIndex].value;
            auto entityIdStream = GetEntityIdStream(compositionIndex);

            // Better to do this in reverse order so that identifiers removal is more likely to hit a fast clear.
            for (int32_t i = comp->count - 1; i >= 0; --i)
            {
                m_identifiers.Remove(EntityID(entityIdStream[i], 0u, 0u));
            }
            
            for (auto i = 0u; i < comp->componentCount; ++i)
            {
                comp->components[i].clear(comp->GetStreams()[i], comp->count);
            }

            comp->count = 0u;
        }
    }


    uint32_t* EntityDatabase::GetEntityIdStream(uint32_t compositionIndex)
    {
        auto* comp = &m_compositions[compositionIndex].value;
        constexpr auto entityIdUUID = pk_type_uuid64<uint32_t>;

        for (auto i = 0u; i < comp->componentCount; ++i)
        {
            if (comp->components[i].typeUUID == entityIdUUID)
            {
                return static_cast<uint32_t*>(comp->GetStreams()[i]);
            }
        }

        return nullptr;
    }

    void EntityDatabase::VisitEntity(uint32_t entityId, uint64_t visitorUUID, void* userdata)
    {
        auto identifierIndex = m_identifiers.GetHashIndex(entityId);

        if (identifierIndex != -1)
        {
            const auto identifier = m_identifiers[identifierIndex];
            const auto compositionIndex = identifier.compositionIndex();
            auto* comp = &m_compositions[compositionIndex].value;
            
            for (auto i = 0u; i < comp->visitorCount; ++i)
            {
                if (comp->visitors[i].uuid == visitorUUID)
                {
                    comp->visitors[i].visit(this, &entityId, userdata);
                    break;
                }
            }
        }
    }

    void EntityDatabase::VisitComposition(uint64_t compositionUUID, uint64_t visitorUUID, void* userdata, uint32_t* entityId)
    {
        const auto typeKey = compositionUUID & COMP_MASK;
        const auto compositionIndex = m_compositions.GetIndex(typeKey);

        if (compositionIndex != -1)
        {
            auto* comp = &m_compositions[compositionIndex].value;

            for (auto i = 0u; i < comp->visitorCount; ++i)
            {
                if (comp->visitors[i].uuid == visitorUUID)
                {
                    comp->visitors[i].visit(this, entityId, userdata);
                    break;
                }
            }
        }
    }

    void EntityDatabase::ReserveEntitities(uint32_t compositionIndex, size_t entryCount)
    {
        auto* comp = &m_compositions[compositionIndex].value;
        const auto isEntity = (m_compositions[compositionIndex].key & VIEW_MASK) == 0u;
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
                comp->components[i].move(streams[i], comp->GetStreams()[i], comp->count);
            }
        }

        Memory::Free(comp->buffer);
        comp->buffer = buffer;
        comp->capacity = newCapacity;

        // Very inefficient double loop to update view indices
        // But this only happens once per entity so whatever.
        for (auto i = 0u; i < m_compositions.GetCount() && isNew; ++i)
        {
            if (m_compositions[i].key & VIEW_MASK)
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
            if ((m_compositions[i].key & VIEW_MASK) == 0ull)
            {
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
                if ((m_compositions[i].key & VIEW_MASK) == 0ull)
                {
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
            }

            Memory::Free(view->buffer);
            view->buffer = newIndices;
            view->capacity = newCount;
        }

        view->count = newCount;
    }
}
