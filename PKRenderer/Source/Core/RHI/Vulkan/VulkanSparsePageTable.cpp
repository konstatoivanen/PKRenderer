#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanSparsePageTable.h"

namespace PK
{
    VulkanSparsePageTable::Page* VulkanSparsePageTable::CreatePage(Page* next, uint32_t beg, uint32_t end, std::vector<VkSparseMemoryBind>& outBindIfos)
    {
        auto page = m_pages.New();
        const auto alignedBeg = beg * m_memoryRequirements.alignment;
        const auto alugnedEnd = end * m_memoryRequirements.alignment;
        
        FixedString128 name("%s.Page(%lli-%lli)", m_name.c_str(), alignedBeg, alugnedEnd);
        
        VkMemoryRequirements requirements{};
        requirements.size = alugnedEnd - alignedBeg;
        requirements.alignment = m_memoryRequirements.alignment;
        requirements.memoryTypeBits = m_memoryRequirements.memoryTypeBits;

        VmaAllocationInfo allocationInfo{};
        VK_ASSERT_RESULT_CTX(vmaAllocateMemoryPages(m_driver->allocator, &requirements, &m_pageCreateInfo, 1, &page->memory, &allocationInfo), "Failed to allocate memory page!");
        VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)allocationInfo.deviceMemory, name.c_str());

        page->beg = beg;
        page->end = end;
        page->next = next;

        VkSparseMemoryBind bind{};
        bind.resourceOffset = alignedBeg;
        bind.size = alugnedEnd - alignedBeg;
        bind.memory = allocationInfo.deviceMemory;
        bind.memoryOffset = allocationInfo.offset;
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
    
        auto next = &m_firstPage;

        for (auto curr = *next; curr; curr = *next)
        {
            *next = curr->next;
            vmaFreeMemoryPages(m_driver->allocator, 1, &curr->memory);
            m_pages.Delete(curr);
        }
    }

    void VulkanSparsePageTable::AllocateRange(const BufferIndexRange& range, QueueType type)
    {
        const auto alignment = m_memoryRequirements.alignment;
        const auto beg = (uint32_t)(range.offset / alignment);
        const auto end = (uint32_t)((range.offset + range.count + alignment - 1) / alignment);

        m_residency.Reserve(range.offset, range.offset + range.count);

        std::vector<VkSparseMemoryBind> bindInfos;

        auto head = 0u;
        auto next = &m_firstPage;

        for (auto curr = *next; curr && curr->beg < end; head = curr->end, curr = *next)
        {
            if (curr->end > beg || curr->beg == head)
            {
                next = &curr->next;
                continue;
            }

            const auto pageBeg = head > beg ? head : beg;
            const auto pageEnd = end < curr->beg ? end : curr->beg;
            *next = CreatePage(curr, pageBeg, pageEnd, bindInfos);
            next = &curr->next;
        }

        // Append list end if range exceeded
        if (head < end)
        {
            auto pageBeg = head > beg ? head : beg;
            auto pageEnd = end;
            *next = CreatePage(*next, pageBeg, pageEnd, bindInfos);
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

    void VulkanSparsePageTable::DeallocateRange(const BufferIndexRange& range)
    {
        const auto alignment = m_memoryRequirements.alignment;
        const auto beg = (uint32_t)(range.offset / alignment);
        const auto end = (uint32_t)((range.offset + range.count + alignment - 1) / alignment);

        m_residency.Unreserve(range.offset, range.offset + range.count);

        auto next = &m_firstPage;

        for (auto curr = *next; curr && curr->beg < end; curr = *next)
        {
            if (curr->end <= beg)
            {
                next = &curr->next;
                continue;
            }

            const auto currAlignedBeg = curr->beg * alignment;
            const auto currAlignedEnd = curr->end * alignment;

            if (!m_residency.IsReservedAny(currAlignedBeg, currAlignedEnd))
            {
                *next = curr->next;
                vmaFreeMemoryPages(m_driver->allocator, 1, &curr->memory);
                m_pages.Delete(curr);
                continue;
            }

            next = &curr->next;
        }
    }
}
