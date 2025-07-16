#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanStagingBufferCache.h"

namespace PK
{
    VulkanStagingBufferCache::VulkanStagingBufferCache(VkDevice device, VmaAllocator allocator, uint64_t pruneDelay) :
        m_allocator(allocator),
        m_device(device),
        m_pruneDelay(pruneDelay)
    {
    }

    VulkanStagingBufferCache::~VulkanStagingBufferCache()
    {
        while (m_freeBufferHead)
        {
            auto next = m_freeBufferHead->next;
            m_bufferPool.Delete(m_freeBufferHead);
            m_freeBufferHead = next;
        }

        while (m_liveBufferHead)
        {
            auto next = m_liveBufferHead->next;
            m_bufferPool.Delete(m_liveBufferHead);
            m_liveBufferHead = next;
        }
    }

    VulkanStagingBuffer* VulkanStagingBufferCache::Acquire(size_t size, bool persistent, const char* name)
    {
        VulkanStagingBuffer* buffer = nullptr;

        if (persistent)
        {
            VulkanBufferCreateInfo createInfo(BufferUsage::DefaultStaging | BufferUsage::PersistentStage, size * PK_RHI_MAX_FRAMES_IN_FLIGHT);
            FixedString128 bufferName("%s.StagingBuffer", name);
            buffer = m_bufferPool.New(m_device, m_allocator, createInfo, bufferName.c_str());
        }
        else
        {
            // Find minimum size free buffer
            for (auto headFree = &m_freeBufferHead; *headFree;)
            {
                if ((*headFree)->size >= size)
                {
                    buffer = *headFree;
                    *headFree = (*headFree)->next;
                    break;
                }

                headFree = &(*headFree)->next;
            }

            // No buffer found. create new.
            if (buffer == nullptr)
            {
                FixedString64 bufferName("StagingBuffer%u", m_bufferPool.GetActiveMask().CountBits());
                VulkanBufferCreateInfo createInfo(BufferUsage::DefaultStaging, size);
                buffer = m_bufferPool.New(m_device, m_allocator, createInfo, bufferName.c_str());
            }
        }

        buffer->pruneTick = ~0ull;
        return buffer;
    }

    void VulkanStagingBufferCache::Release(VulkanStagingBuffer* buffer, const FenceRef& fence)
    {
        if (buffer == nullptr)
        {
            return;
        }

        auto nextPruneTick = m_currentPruneTick + 1;
        buffer->pruneTick = nextPruneTick;
        buffer->fence = fence;

        if (buffer->isPersistentMap)
        {
            auto deleter = [](void* v)
            {
                RHIDriver::Get()->GetNative<VulkanDriver>()->stagingBufferCache->m_bufferPool.Delete(reinterpret_cast<VulkanStagingBuffer*>(v));
            };

            RHIDriver::Get()->GetNative<VulkanDriver>()->disposer->Dispose(buffer, deleter, fence);
        }
        else
        {
            buffer->next = m_liveBufferHead;
            m_liveBufferHead = buffer;
        }
    }

    void VulkanStagingBufferCache::Prune()
    {
        ++m_currentPruneTick;

        for (auto headLive = &m_liveBufferHead; *headLive;)
        {
            auto buffer = *headLive;

            // If staging buffer has been assigned an execution observer let's wait for that instead of prune tick.
            if (buffer->fence.IsValid() ? buffer->fence.IsComplete() : buffer->pruneTick < m_currentPruneTick)
            {
                *headLive = buffer->next;
                buffer->fence.Invalidate();
                buffer->pruneTick = m_currentPruneTick + m_pruneDelay;
                buffer->next = nullptr;

                // Insertion order sort
                // A lot slower than doing an array sort, but this is used so sparsely. saving the allocations is more beneficial.
                for (auto lowerHead = &m_liveBufferHead; true;)
                {
                    auto other = *lowerHead;

                    if (other == nullptr || other->size < buffer->size || (other->size == buffer->size && other->pruneTick >= buffer->pruneTick))
                    {
                        buffer->next = other;
                        *lowerHead = buffer;
                        break;
                    }

                    lowerHead = &other->next;
                }

                continue;
            }
            
            headLive = &buffer->next;
        }

        for (auto headFree = &m_freeBufferHead; *headFree;)
        {
            if ((*headFree)->pruneTick < m_currentPruneTick)
            {
                auto next = (*headFree)->next;
                m_bufferPool.Delete(*headFree);
                *headFree = next;
                continue;
            }

            headFree = &(*headFree)->next;
        }
    }
}
