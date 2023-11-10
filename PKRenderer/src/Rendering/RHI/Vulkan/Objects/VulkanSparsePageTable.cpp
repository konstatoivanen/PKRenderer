#include "PrecompiledHeader.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanUtilities.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "VulkanSparsePageTable.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    using namespace PK::Utilities;

    VulkanSparsePageTable::Page::Page(const VmaAllocator allocator, const VkMemoryRequirements& requirements, const VmaAllocationCreateInfo& createInfo) :
        allocator(allocator),
        start(start),
        end(end)
    {
        VK_ASSERT_RESULT_CTX(vmaAllocateMemoryPages(allocator, &requirements, &createInfo, 1, &memory, &allocationInfo), "Failed to allocate memory page!");
        PK_LOG_INFO("New: SparsePage %u bytes", (uint32_t)requirements.size);
    }

    VulkanSparsePageTable::Page::~Page()
    {
        vmaFreeMemoryPages(allocator, 1, &memory);
    }

    VulkanSparsePageTable::Page* VulkanSparsePageTable::CreatePage(Page* next, Page* prev, size_t start, size_t end, std::vector<VkSparseMemoryBind>& outBindIfos)
    {
        VkMemoryRequirements requirements = m_memoryRequirements;
        requirements.size = (end - start);

        auto page = m_pages.New(m_driver->allocator, requirements, m_pageCreateInfo);
        
        page->start = start;
        page->end = end;
        page->next = next;

        if (prev)
        {
            prev->next = page;
        }
        
        VkSparseMemoryBind bind{};
        bind.resourceOffset = start;
        bind.size = end - start;
        bind.memory = page->allocationInfo.deviceMemory;
        bind.memoryOffset = page->allocationInfo.offset;
        outBindIfos.push_back(bind);
        return page;
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
        auto start = (range.offset / alignment) * alignment;
        auto end = ((range.offset + range.count + alignment - 1) / alignment) * alignment;

        std::vector<VkSparseMemoryBind> bindInfos;

        Page* prev = nullptr;
        size_t head = 0ull;
        auto curr = m_firstPage;

        while (curr)
        {
            if (curr->start != head && curr->start > start)
            {
                auto pageStart = head > start ? head : start;
                auto pageEnd = end < curr->start ? end : curr->start;
                auto page = CreatePage(curr, prev, pageStart, pageEnd, bindInfos);
            }

            head = curr->end;
            prev = curr;
            curr = curr->next;
        }

        // Linked list end add page if range exceed
        if (head < end)
        {
            auto pageStart = head > start ? head : start;
            auto pageEnd = end;
            auto page = CreatePage(nullptr, prev, pageStart, pageEnd, bindInfos);

            // Set first page if unset
            if (!m_firstPage)
            {
                m_firstPage = page;
            }
        }

        if (bindInfos.size() > 0)
        {
            auto queue = m_driver->queues->GetQueue(type);
            queue->BindSparse(m_targetBuffer, bindInfos.data(), (uint32_t)bindInfos.size());
        }
    }

    IndexRange VulkanSparsePageTable::AllocateAligned(size_t size, QueueType type)
    {
        auto alignment = m_memoryRequirements.alignment;
        size = ((size + alignment - 1) / alignment) * alignment;

        std::vector<VkSparseMemoryBind> bindInfos;

        Page* prev = nullptr;
        size_t head = 0ull;
        auto curr = m_firstPage;

        while (curr)
        {
            auto space = curr->start - head;

            if (space >= size)
            {
                auto page = CreatePage(curr, prev, head, head + size, bindInfos);
                break;
            }

            head = curr->end;
            prev = curr;
            curr = curr->next;
        }

        // Linked list end add page if range exceed
        if (!curr)
        {
            auto page = CreatePage(nullptr, prev, head, head + size, bindInfos);

            // Set first page if unset
            if (!m_firstPage)
            {
                m_firstPage = page;
            }
        }

        if (bindInfos.size() > 0)
        {
            auto queue = m_driver->queues->GetQueue(type);
            queue->BindSparse(m_targetBuffer, bindInfos.data(), (uint32_t)bindInfos.size());
        }

        return { head, size };
    }

    void VulkanSparsePageTable::FreeRange(const IndexRange& range)
    {
        auto alignment = m_memoryRequirements.alignment;
        auto start = (range.offset / alignment) * alignment;
        auto end = ((range.offset + range.count + alignment - 1) / alignment) * alignment;

        Page* prev = nullptr;
        auto curr = m_firstPage;

        while (curr)
        {
            if (curr->start >= end || curr->end <= start)
            {
                prev = curr;
                curr = curr->next;
                continue;
            }

            PK_WARNING_ASSERT(curr->start >= start && curr->end <= end, "Sub page range deallocation isn't supported!. @TODO FIX THIS!!!");

            auto temp = curr->next;
            m_pages.Delete(curr);

            // Update link
            if (prev)
            {
                prev->next = temp;
            }
            
            curr = temp;
        }
    }

}