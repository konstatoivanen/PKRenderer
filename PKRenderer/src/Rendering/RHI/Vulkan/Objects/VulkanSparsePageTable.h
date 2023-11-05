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
            Page(const VmaAllocator allocator, size_t start, size_t end, const VkMemoryRequirements& memReq, const VmaAllocationCreateInfo& createInfo);
            ~Page();

            const VmaAllocator allocator;
            const size_t start;
            const size_t end;
            VmaAllocation memory;
            VmaAllocationInfo allocationInfo;
        };

        public:
            VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage);
            ~VulkanSparsePageTable();

            void AllocateRange(const IndexRange& range, QueueType type);
            void FreeRange(const IndexRange& range);

        private:
            bool CheckRange(size_t* start, size_t* end);
            
            const VulkanDriver* m_driver = nullptr;
            const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
            VmaAllocationCreateInfo m_pageCreateInfo{};
            VkMemoryRequirements m_memoryRequirements{};
            std::map<uint32_t, Page*> m_activePages;
            PK::Utilities::FixedPool<Page, 1024> m_pages;
    };
}