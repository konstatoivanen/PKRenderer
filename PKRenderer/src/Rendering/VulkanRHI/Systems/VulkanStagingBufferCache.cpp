#include "PrecompiledHeader.h"
#include "VulkanStagingBufferCache.h"
#include "Utilities/VectorUtilities.h"
#include "Core/Services/Log.h"

using namespace PK::Utilities;
using namespace PK::Rendering::VulkanRHI::Systems;

template<>
struct Vector::Comparer<VulkanStagingBuffer*>
{
    int operator ()(VulkanStagingBuffer*& a, VulkanStagingBuffer*& b)
    {
        if (a->capacity < b->capacity)
        {
            return -1;
        }

        if (a->capacity > b->capacity)
        {
            return 1;
        }

        if (a->pruneTick < b->pruneTick)
        {
            return -1;
        }

        if (a->pruneTick > b->pruneTick)
        {
            return 1;
        }

        return 0;
    }
};

template<>
struct Vector::Bound<VulkanStagingBuffer*>
{
    size_t operator ()(VulkanStagingBuffer*& a)
    {
        return a->capacity;
    }
};

namespace PK::Rendering::VulkanRHI::Systems
{
    VulkanStagingBufferCache::~VulkanStagingBufferCache()
    {
        for (auto& buff : m_freeBuffers)
        {
            m_bufferPool.Delete(buff);
        }

        for (auto& buff : m_activeBuffers)
        {
            m_bufferPool.Delete(buff);
        }
    }

    VulkanStagingBuffer* VulkanStagingBufferCache::GetBuffer(size_t size)
    {
        auto index = Vector::LowerBound(m_freeBuffers, (uint32_t)size);
        auto nextPruneTick = m_currentPruneTick + 1;

        if (index != -1)
        {
            auto stagingBuffer = m_freeBuffers.at(index);
            stagingBuffer->pruneTick = nextPruneTick;
            Vector::OrderedRemoveAt(m_freeBuffers, index);
            m_activeBuffers.push_back(stagingBuffer);
            return stagingBuffer;
        }

        VulkanBufferCreateInfo createInfo(BufferUsage::DefaultStaging, size);
        auto stagingBuffer = m_bufferPool.New(m_allocator, createInfo);
        stagingBuffer->pruneTick = nextPruneTick;
        stagingBuffer->executionGate.Invalidate();
        m_activeBuffers.push_back(stagingBuffer);
        return stagingBuffer;
    }
    
    void VulkanStagingBufferCache::Prune()
    {
        ++m_currentPruneTick;

        for (auto i = (int)m_activeBuffers.size() - 1; i >= 0; --i)
        {
            auto stagingBuffer = m_activeBuffers.at(i);

            // If staging buffer has been assigned an execution observer let's wait for that instead of prune tick.
            if (stagingBuffer->executionGate.IsValid() ? stagingBuffer->executionGate.IsCompleted() : stagingBuffer->pruneTick < m_currentPruneTick)
            {
                stagingBuffer->executionGate.Invalidate();
                stagingBuffer->pruneTick = m_currentPruneTick + m_pruneDelay;
                m_freeBuffers.push_back(stagingBuffer);
                Vector::UnorderedRemoveAt(m_activeBuffers, i);
            }
        }

        for (auto i = (int)m_freeBuffers.size() - 1; i >= 0; --i)
        {
            auto stagingBuffer = m_freeBuffers.at(i);

            if (stagingBuffer->pruneTick < m_currentPruneTick)
            {
                m_bufferPool.Delete(stagingBuffer);
                Vector::UnorderedRemoveAt(m_freeBuffers, i);
            }
        }

        Vector::QuickSort(m_freeBuffers);
    }
}