#include "PrecompiledHeader.h"
#include "Utilities/VectorUtilities.h"
#include "Core/Services/Log.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "VulkanStagingBufferCache.h"

using namespace PK::Utilities;
using namespace PK::Rendering::RHI::Vulkan::Services;

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

namespace PK::Rendering::RHI::Vulkan::Services
{

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

        // @TODO refactor to be non static
        static uint64_t s_bufferIdCounter = 0ull;

        if (persistent)
        {
            VulkanBufferCreateInfo createInfo(BufferUsage::DefaultStaging | BufferUsage::PersistentStage, size * PK_MAX_FRAMES_IN_FLIGHT);
            buffer = m_bufferPool.New(m_device, m_allocator, createInfo, (std::string(name) + std::string(".StagingBuffer")).c_str());
        }
        else
        {
            auto index = Vector::LowerBound(m_freeBuffers, (uint32_t)size);

            if (index != -1)
            {
                buffer = m_freeBuffers.at(index);
                Vector::OrderedRemoveAt(m_freeBuffers, index);
            }
            else
            {
                VulkanBufferCreateInfo createInfo(BufferUsage::DefaultStaging, size);
                buffer = m_bufferPool.New(m_device, m_allocator, createInfo, (std::string("StagingBuffer") + std::to_string(s_bufferIdCounter++)).c_str());
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
                RHI::Driver::GetNative<VulkanDriver>()->stagingBufferCache->m_bufferPool.Delete(reinterpret_cast<VulkanStagingBuffer*>(v));
            };

            RHI::Driver::GetNative<VulkanDriver>()->disposer->Dispose(buffer, deleter, fence);
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