#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/Structs/StructsCommon.h"
#include "Utilities/FixedPool.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;

    class VulkanSparsePageTable : public IVulkanDisposable
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

            void AllocateRange(const IndexRange& range);
            void FreeRange(const IndexRange& range);

        private:
            bool CheckRange(size_t* start, size_t* end);
            
            const VulkanDriver* m_driver = nullptr;
            const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
            VkQueue m_queue = VK_NULL_HANDLE;
            VmaAllocationCreateInfo m_pageCreateInfo{};
            VkMemoryRequirements m_memoryRequirements{};
            std::map<uint32_t, Page*> m_activePages;
            FixedPool<Page, 1024> m_pages;
    };
}