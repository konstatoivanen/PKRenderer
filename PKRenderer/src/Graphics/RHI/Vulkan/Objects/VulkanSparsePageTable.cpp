#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Graphics/RHI/Vulkan/VulkanDriver.h"
#include "VulkanSparsePageTable.h"

namespace PK::Graphics::RHI::Vulkan::Objects
{
    using namespace PK::Utilities;

    VulkanSparsePageTable::Page::Page(const VkDevice device,
        const VmaAllocator allocator,
        const VkMemoryRequirements& requirements,
        const VmaAllocationCreateInfo& createInfo,
        const char* name) :
        allocator(allocator)
    {
        VK_ASSERT_RESULT_CTX(vmaAllocateMemoryPages(allocator, &requirements, &createInfo, 1, &memory, &allocationInfo), "Failed to allocate memory page!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)allocationInfo.deviceMemory, name);
    }

    VulkanSparsePageTable::Page::~Page()
    {
        vmaFreeMemoryPages(allocator, 1, &memory);
    }

    VulkanSparsePageTable::Page* VulkanSparsePageTable::CreatePage(Page* next, size_t start, size_t end, std::vector<VkSparseMemoryBind>& outBindIfos)
    {
        auto name = m_name + std::string(".Page(") + std::to_string(start) + std::string("-") + std::to_string(end) + std::string(")");

        auto page = m_pages.New(m_driver->device,
            m_driver->allocator,
            VkMemoryRequirements
            {
                end - start,
                m_memoryRequirements.alignment,
                m_memoryRequirements.memoryTypeBits
            },
            m_pageCreateInfo,
            name.c_str());

        page->start = start;
        page->end = end;
        page->next = next;

        VkSparseMemoryBind bind{};
        bind.resourceOffset = start;
        bind.size = end - start;
        bind.memory = page->allocationInfo.deviceMemory;
        bind.memoryOffset = page->allocationInfo.offset;
        outBindIfos.push_back(bind);
        return page;
    }

    VulkanSparsePageTable::VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage, const char* name) :
        m_driver(driver),
        m_targetBuffer(buffer),
        m_name(name)
    {
        m_pageCreateInfo.usage = memoryUsage;
        vkGetBufferMemoryRequirements(m_driver->device, buffer, &m_memoryRequirements);
    }

    VulkanSparsePageTable::~VulkanSparsePageTable()
    {
        PK_WARNING_ASSERT(m_firstPage == nullptr, "not all ranges were deallocated!");
    }

    void VulkanSparsePageTable::AllocateRange(const IndexRange& range, QueueType type)
    {
        auto alignment = m_memoryRequirements.alignment;
        auto start = (range.offset / alignment) * alignment;
        auto end = ((range.offset + range.count + alignment - 1) / alignment) * alignment;

        m_residency.Reserve(range.offset, range.offset + range.count);

        std::vector<VkSparseMemoryBind> bindInfos;

        size_t head = 0ull;
        auto next = &m_firstPage;

        for (auto curr = *next; curr && curr->start < end; head = curr->end, curr = *next)
        {
            if (curr->end > start || curr->start == head)
            {
                next = &curr->next;
                continue;
            }

            auto pageStart = head > start ? head : start;
            auto pageEnd = end < curr->start ? end : curr->start;

            // Fill hole
            *next = CreatePage(curr, pageStart, pageEnd, bindInfos);
            next = &curr->next;
        }

        // Append list end if range exceeded
        if (head < end)
        {
            auto pageStart = head > start ? head : start;
            auto pageEnd = end;
            *next = CreatePage(*next, pageStart, pageEnd, bindInfos);
        }

        if (bindInfos.size() > 0)
        {
            auto queue = m_driver->queues->GetQueue(type);
            queue->BindSparse(m_targetBuffer, bindInfos.data(), (uint32_t)bindInfos.size());
        }
    }

    size_t VulkanSparsePageTable::Allocate(size_t size, QueueType type)
    {
        size_t offset = m_residency.FindFreeOffset(size);
        AllocateRange({ offset, size }, type);
        return offset;
    }

    void VulkanSparsePageTable::DeallocateRange(const IndexRange& range)
    {
        auto alignment = m_memoryRequirements.alignment;
        auto start = (range.offset / alignment) * alignment;
        auto end = ((range.offset + range.count + alignment - 1) / alignment) * alignment;

        m_residency.Unreserve(range.offset, range.offset + range.count);

        auto next = &m_firstPage;

        for (auto curr = *next; curr && curr->start < end; curr = *next)
        {
            if (curr->end <= start)
            {
                next = &curr->next;
                continue;
            }

            if (!m_residency.IsReservedAny(curr->start, curr->end))
            {
                *next = curr->next;
                m_pages.Delete(curr);
                continue;
            }

            next = &curr->next;
        }
    }

}