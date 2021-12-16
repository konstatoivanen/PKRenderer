#include "PrecompiledHeader.h"
#include "VulkanStagingBufferCache.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    VulkanStagingBufferCache::~VulkanStagingBufferCache()
    {
        for (auto& kv : m_freeBuffers)
        {
            delete kv.second;
        }

        for (auto stagingBuffer : m_activeBuffers)
        {
            delete stagingBuffer;
        }
    }

    const VulkanStagingBuffer* VulkanStagingBufferCache::GetBuffer(size_t size)
    {
        auto iter = m_freeBuffers.lower_bound(size);
        auto nextPruneTick = m_currentPruneTick + 1;

        if (iter != m_freeBuffers.end())
        {
            auto stagingBuffer = iter->second;
            stagingBuffer->pruneTick = nextPruneTick;
            m_freeBuffers.erase(iter);
            m_activeBuffers.insert(stagingBuffer);
            return stagingBuffer;
        }

        VulkanBufferCreateInfo createInfo(BufferUsage::Staging, size);
        auto stagingBuffer = new VulkanStagingBuffer(m_allocator, createInfo);
        stagingBuffer->pruneTick = nextPruneTick;
        stagingBuffer->executionGate.Invalidate();
        m_activeBuffers.insert(stagingBuffer);
        return stagingBuffer;
    }
    
    void VulkanStagingBufferCache::Prune()
    {
        ++m_currentPruneTick;

        decltype(m_freeBuffers) freeBuffers;
        freeBuffers.swap(m_freeBuffers);

        for (auto& kv : freeBuffers)
        {
            if (kv.second->pruneTick > m_currentPruneTick) 
            {
                m_freeBuffers.insert(kv);
                continue;
            }

            delete kv.second;
        }
        
        decltype(m_activeBuffers) activeBuffers;
        activeBuffers.swap(m_activeBuffers);
        
        for (auto& stagingBuffer : activeBuffers)
        {
            // If staging buffer has been assigned an execution observer let's wait for that instead of prune tick.
            if (stagingBuffer->executionGate.IsValid() ? stagingBuffer->executionGate.IsCompleted() : stagingBuffer->pruneTick < m_currentPruneTick)
            {
                stagingBuffer->executionGate.Invalidate();
                stagingBuffer->pruneTick = m_currentPruneTick + m_pruneDelay;
                m_freeBuffers.insert(std::make_pair(stagingBuffer->capacity, stagingBuffer));
                continue;
            }

            m_activeBuffers.insert(stagingBuffer);
        }
    }
}