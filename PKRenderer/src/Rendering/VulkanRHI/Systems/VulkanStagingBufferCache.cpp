#include "PrecompiledHeader.h"
#include "VulkanStagingBufferCache.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    const VulkanStagingBuffer* VulkanStagingBufferCache::GetBuffer(size_t size)
    {
        auto iter = m_freeBuffers.lower_bound(size);

        if (iter != m_freeBuffers.end())
        {
            auto stagingBuffer = iter->second;
            m_freeBuffers.erase(iter);
            m_activeBuffers.insert(stagingBuffer);
            return stagingBuffer.get();
        }

        VulkanBufferCreateInfo createInfo(BufferUsage::Staging, size);
        auto stagingBuffer = CreateRef<VulkanStagingBuffer>(m_allocator, createInfo);
        stagingBuffer->pruneTick = m_currentPruneTick + m_pruneDelay;
        stagingBuffer->executionGate.Invalidate();
        m_activeBuffers.insert(stagingBuffer);
        return stagingBuffer.get();
    }
    
    void VulkanStagingBufferCache::Prune(bool all)
    {
        if (all)
        {
            m_freeBuffers.clear();
            m_activeBuffers.clear();
            return;
        }

        ++m_currentPruneTick;

        decltype(m_freeBuffers) freeBuffers;
        freeBuffers.swap(m_freeBuffers);

        for (auto& kv : freeBuffers)
        {
            if (kv.second->pruneTick > m_currentPruneTick) 
            {
                m_freeBuffers.insert(kv);
            }
        }
        
        decltype(m_activeBuffers) activeBuffers;
        activeBuffers.swap(m_activeBuffers);
        
        for (auto& stagingBuffer : activeBuffers)
        {
            // If staging buffer has been assigned an execution observer let's wait for that instead of prune tick.
            if (stagingBuffer->executionGate.IsValid() ? stagingBuffer->executionGate.IsCompleted() : stagingBuffer->pruneTick < m_currentPruneTick)
            {
                stagingBuffer->executionGate.Invalidate();
                stagingBuffer->pruneTick = m_currentPruneTick;
                m_freeBuffers.insert(std::make_pair(stagingBuffer->capacity, stagingBuffer));
            }
            else 
            {
                m_activeBuffers.insert(stagingBuffer);
            }
        }
    }
}