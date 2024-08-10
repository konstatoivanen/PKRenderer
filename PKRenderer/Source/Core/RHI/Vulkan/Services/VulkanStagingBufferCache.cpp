#include "PrecompiledHeader.h"
#include "Core/Utilities/VectorHelpers.h"
#include "Core/Utilities/Parsing.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanStagingBufferCache.h"

namespace PK
{
    template<>
    struct VectorHelpers::Comparer<VulkanStagingBuffer*>
    {
        int operator ()(VulkanStagingBuffer*& a, VulkanStagingBuffer*& b)
        {
            if (a->size < b->size)
            {
                return -1;
            }

            if (a->size > b->size)
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
    struct VectorHelpers::Bound<VulkanStagingBuffer*>
    {
        size_t operator ()(VulkanStagingBuffer*& a)
        {
            return a->size;
        }
    };

    VulkanStagingBufferCache::VulkanStagingBufferCache(VkDevice device, VmaAllocator allocator, uint64_t pruneDelay) :
        m_allocator(allocator),
        m_device(device),
        m_pruneDelay(pruneDelay)
    {
        m_activeBuffers.reserve(32);
        m_freeBuffers.reserve(32);
    }


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

    VulkanStagingBuffer* VulkanStagingBufferCache::Acquire(size_t size, bool persistent, const char* name)
    {
        VulkanStagingBuffer* buffer = nullptr;

        if (persistent)
        {
            VulkanBufferCreateInfo createInfo(BufferUsage::DefaultStaging | BufferUsage::PersistentStage, size * PK_RHI_MAX_FRAMES_IN_FLIGHT);
            auto bufferName = Parse::FormatToString("%s.StagingBuffer", name);
            buffer = m_bufferPool.New(m_device, m_allocator, createInfo, bufferName.c_str());
        }
        else
        {
            auto index = VectorHelpers::LowerBound(m_freeBuffers, (uint32_t)size);

            if (index != -1)
            {
                buffer = m_freeBuffers.at(index);
                VectorHelpers::OrderedRemoveAt(m_freeBuffers, index);
            }
            else
            {
                auto bufferName = Parse::FormatToString("StagingBuffer%u", m_bufferPool.GetActiveMask().CountBits());
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

        if (buffer->persistentmap)
        {
            auto deleter = [](void* v)
            {
                RHIDriver::Get()->GetNative<VulkanDriver>()->stagingBufferCache->m_bufferPool.Delete(reinterpret_cast<VulkanStagingBuffer*>(v));
            };

            RHIDriver::Get()->GetNative<VulkanDriver>()->disposer->Dispose(buffer, deleter, fence);
        }
        else
        {
            m_activeBuffers.push_back(buffer);
        }
    }

    void VulkanStagingBufferCache::Prune()
    {
        ++m_currentPruneTick;

        for (auto i = (int)m_activeBuffers.size() - 1; i >= 0; --i)
        {
            auto stagingBuffer = m_activeBuffers.at(i);

            // If staging buffer has been assigned an execution observer let's wait for that instead of prune tick.
            if (stagingBuffer->fence.IsValid() ? stagingBuffer->fence.IsComplete() : stagingBuffer->pruneTick < m_currentPruneTick)
            {
                stagingBuffer->fence.Invalidate();
                stagingBuffer->pruneTick = m_currentPruneTick + m_pruneDelay;
                m_freeBuffers.push_back(stagingBuffer);
                VectorHelpers::UnorderedRemoveAt(m_activeBuffers, i);
            }
        }

        for (auto i = (int)m_freeBuffers.size() - 1; i >= 0; --i)
        {
            auto stagingBuffer = m_freeBuffers.at(i);

            if (stagingBuffer->pruneTick < m_currentPruneTick)
            {
                m_bufferPool.Delete(stagingBuffer);
                VectorHelpers::UnorderedRemoveAt(m_freeBuffers, i);
            }
        }

        VectorHelpers::QuickSort(m_freeBuffers);
    }
}