#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/Structs/StructsCommon.h"
#include "Utilities/FixedPool.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanSparsePageTable : public Rendering::Services::IDisposable
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

            void AllocateRange(const Structs::IndexRange& range);
            void FreeRange(const Structs::IndexRange& range);

        private:
            bool CheckRange(size_t* start, size_t* end);
            
            const VulkanDriver* m_driver = nullptr;
            const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
            VkQueue m_queue = VK_NULL_HANDLE;
            VmaAllocationCreateInfo m_pageCreateInfo{};
            VkMemoryRequirements m_memoryRequirements{};
            std::map<uint32_t, Page*> m_activePages;
            PK::Utilities::FixedPool<Page, 1024> m_pages;
    };
}