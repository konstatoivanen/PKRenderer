#include "PrecompiledHeader.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanUtilities.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "VulkanSparsePageTable.h"

namespace PK::Rendering::RHI::Vulkan::Objects
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
    }

    VulkanSparsePageTable::~VulkanSparsePageTable()
    {
    }

    void VulkanSparsePageTable::AllocateRange(const IndexRange& range, QueueType type)
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
            m_activePages[(uint32_t)end] = page;
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

        auto queue = m_driver->queues->GetQueue(type);
        queue->BindSparse(m_targetBuffer, bindInfos.data(), (uint32_t)bindInfos.size());
    }

    void VulkanSparsePageTable::FreeRange(const IndexRange& range)
    {
        auto alignment = m_memoryRequirements.alignment;
        auto start = range.offset / alignment;
        auto end = (range.offset + range.count + alignment - 1) / alignment;
        auto iter = m_activePages.upper_bound((uint32_t)start);

        while (iter != m_activePages.end() && iter->second->end <= end)
        {
            m_pages.Delete(iter->second);
            m_activePages.erase(iter->first);
            iter = m_activePages.upper_bound((uint32_t)start);
        }
    }

    bool VulkanSparsePageTable::CheckRange(size_t* start, size_t* end)
    {
        if (*start >= *end)
        {
            return false;
        }

        auto iter = m_activePages.lower_bound((uint32_t)*start);

        while (iter != m_activePages.end())
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