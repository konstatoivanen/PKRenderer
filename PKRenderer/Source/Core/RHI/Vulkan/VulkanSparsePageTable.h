#pragma once
#include "Core/Utilities/RangeTable.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;

    struct VulkanSparsePageTable : public VersionedObject
    {
        VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage, const char* name);
        ~VulkanSparsePageTable();

        void AllocateRange(const BufferIndexRange& range, QueueType type);
        size_t Allocate(size_t size, QueueType type);
        void DeallocateRange(const BufferIndexRange& range);

    private:
        struct Page
        {
            VmaAllocation memory = VK_NULL_HANDLE;
            Page* next = nullptr;
            uint32_t beg = 0u;
            uint32_t end = 0u;
        };

        Page* CreatePage(Page* next, uint32_t start, uint32_t end);
        
        VmaAllocationCreateInfo m_pageCreateInfo{};
        VkMemoryRequirements m_memoryRequirements{};

        const VulkanDriver* m_driver = nullptr;
        const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
        Page* m_firstPage = nullptr;
        FixedPool<Page, PK_VK_MAX_SPARSE_PAGES> m_pages;
        FixedRangeTable<PK_VK_MAX_SPARSE_RANGES> m_residency;
        FixedString128 m_name;
    };
}
