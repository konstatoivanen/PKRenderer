#pragma once
#include "Utilities/FixedPool.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanSparsePageTable : public PK::Utilities::VersionedObject
    {
        struct Page
        {
            Page(const VmaAllocator allocator, const VkMemoryRequirements& requirements, const VmaAllocationCreateInfo& createInfo);
            ~Page();

            const VmaAllocator allocator;
            VmaAllocation memory;
            VmaAllocationInfo allocationInfo;

            // Linked list info
            Page* next = nullptr;
            size_t start = 0ull;
            size_t end = 0ull;
        };

        Page* CreatePage(Page* next, Page* prev, size_t start, size_t end, std::vector<VkSparseMemoryBind>& outBindIfos);
        
        public:
            VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage);
            ~VulkanSparsePageTable();

            void AllocateRange(const IndexRange& range, QueueType type);
            IndexRange AllocateAligned(size_t size, QueueType type);
            void FreeRange(const IndexRange& range);

        private:
            VmaAllocationCreateInfo m_pageCreateInfo{};
            VkMemoryRequirements m_memoryRequirements{};

            const VulkanDriver* m_driver = nullptr;
            const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
            Page* m_firstPage = nullptr;
            PK::Utilities::FixedPool<Page, 1024> m_pages;
    };
}