#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FixedRangeTable.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;

    class VulkanSparsePageTable : public VersionedObject
    {
        struct Page
        {
            VmaAllocation memory = VK_NULL_HANDLE;
            Page* next = nullptr;
            uint32_t beg = 0u;
            uint32_t end = 0u;
        };

        Page* CreatePage(Page* next, uint32_t start, uint32_t end, std::vector<VkSparseMemoryBind>& outBindIfos);
        
        public:
            VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage, const char* name);
            ~VulkanSparsePageTable();

            void AllocateRange(const BufferIndexRange& range, QueueType type);
            size_t Allocate(size_t size, QueueType type);
            void DeallocateRange(const BufferIndexRange& range);

        private:
            VmaAllocationCreateInfo m_pageCreateInfo{};
            VkMemoryRequirements m_memoryRequirements{};

            const VulkanDriver* m_driver = nullptr;
            const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
            Page* m_firstPage = nullptr;
            FixedPool<Page, 1024> m_pages;
            FixedRangeTable<1024> m_residency;
            FixedString128 m_name;
    };
}
