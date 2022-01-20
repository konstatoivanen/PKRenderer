#include "PrecompiledHeader.h"
#include "VulkanSparsePageTable.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;

    VulkanSparsePageTable::Page::Page(VmaAllocator allocator,
                                      size_t start, 
                                      size_t end,
                                      const VkMemoryRequirements& memoryRequirements, 
                                      const VmaAllocationCreateInfo& createInfo) :
        allocator(allocator),
        start(start),
        end(end)
    {
        VkMemoryRequirements memreq = memoryRequirements;
        memreq.size = (end - start) * memoryRequirements.alignment;
        VK_ASSERT_RESULT_CTX(vmaAllocateMemoryPages(allocator, &memreq, &createInfo, 1, &memory, &allocationInfo), "Failed to allocate memory page!");
    }

    VulkanSparsePageTable::Page::~Page()
    {
        vmaFreeMemoryPages(allocator, 1, &memory);
    }

    VulkanSparsePageTable::VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage) :
        m_driver(driver),
        m_targetBuffer(buffer)
    {
        m_pageCreateInfo.usage = memoryUsage;
        vkGetBufferMemoryRequirements(m_driver->device, buffer, &m_memoryRequirements);
        vkGetDeviceQueue(m_driver->device, m_driver->queueFamilies[QueueType::Graphics], 0, &m_queue);
    }

    VulkanSparsePageTable::~VulkanSparsePageTable()
    {
    }

    void VulkanSparsePageTable::AllocateRange(const IndexRange& range)
    {
        auto alignment = m_memoryRequirements.alignment;
        auto low = range.offset / alignment;
        auto high = (range.offset + range.count + alignment - 1) / alignment;
        size_t start = low;
        size_t end = high;

        std::vector<VkSparseMemoryBind> bindInfos;

        while (CheckRange(&start, &end))
        {
            auto page = m_pages.New(m_driver->allocator, start, end, m_memoryRequirements, m_pageCreateInfo);
            m_activeBlocks[(uint32_t)end] = page;
            VkSparseMemoryBind bind{};
            bind.resourceOffset = page->start * alignment;
            bind.size = (page->end - page->start) * alignment;
            bind.memory = page->allocationInfo.deviceMemory;
            bind.memoryOffset = page->allocationInfo.offset;
            bindInfos.push_back(bind);
            start = end;
            end = high;
        }

        if (bindInfos.size() == 0)
        {
            return;
        }

        VkBindSparseInfo sparseBind{ VK_STRUCTURE_TYPE_BIND_SPARSE_INFO };
        VkSparseBufferMemoryBindInfo bufferBind{};
        auto signal = m_driver->commandBufferPool->QueueDependency(&sparseBind.pWaitSemaphores);
        bufferBind.buffer = m_targetBuffer;
        bufferBind.bindCount = (uint32_t)bindInfos.size();
        bufferBind.pBinds = bindInfos.data();
        sparseBind.waitSemaphoreCount = sparseBind.pWaitSemaphores ? 1 : 0;
        sparseBind.bufferBindCount = 1;
        sparseBind.pBufferBinds = &bufferBind;
        sparseBind.signalSemaphoreCount = 1;
        sparseBind.pSignalSemaphores = &signal;
        vkQueueBindSparse(m_queue, 1, &sparseBind, VK_NULL_HANDLE);
    }

    void VulkanSparsePageTable::FreeRange(const IndexRange& range)
    {
        auto alignment = m_memoryRequirements.alignment;
        auto start = range.offset / alignment;
        auto end = (range.offset + range.count + alignment - 1) / alignment;
        auto iter = m_activeBlocks.upper_bound((uint32_t)start);

        while (iter != m_activeBlocks.end() && iter->second->end <= end)
        {
            m_pages.Delete(iter->second);
            m_activeBlocks.erase(iter->first);
            iter = m_activeBlocks.upper_bound((uint32_t)start);
        }
    }

    bool VulkanSparsePageTable::CheckRange(size_t* start, size_t* end)
    {
        if (*start >= *end)
        {
            return false;
        }

        auto iter = m_activeBlocks.lower_bound((uint32_t)*start);

        while (iter != m_activeBlocks.end())
        {
            auto page = iter->second;

            if (page->start > *start)
            {
                *end = page->start;
                return false;
            }

            if (page->end >= *end)
            {
                return true;
            }

            *start = page->end;
            ++iter;
        }

        return true;
    }
}