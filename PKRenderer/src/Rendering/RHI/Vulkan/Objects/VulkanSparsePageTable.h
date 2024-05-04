#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/FixedPool.h"
#include "Utilities/RangeTable.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Vulkan/VulkanCommon.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct VulkanDriver)

namespace PK::Rendering::RHI::Vulkan::Objects
{
    class VulkanSparsePageTable : public PK::Utilities::VersionedObject
    {
        struct Page
        {
            Page(const VkDevice device,
                const VmaAllocator allocator,
                const VkMemoryRequirements& requirements,
                const VmaAllocationCreateInfo& createInfo,
                const char* name);
            ~Page();

            const VmaAllocator allocator;
            VmaAllocation memory;
            VmaAllocationInfo allocationInfo;

            // Linked list info
            Page* next = nullptr;
            size_t start = 0ull;
            size_t end = 0ull;
        };

        Page* CreatePage(Page* next, size_t start, size_t end, std::vector<VkSparseMemoryBind>& outBindIfos);
        
        public:
            VulkanSparsePageTable(const VulkanDriver* driver, const VkBuffer buffer, VmaMemoryUsage memoryUsage, const char* name);
            ~VulkanSparsePageTable();

            void AllocateRange(const IndexRange& range, QueueType type);
            size_t Allocate(size_t size, QueueType type);
            void DeallocateRange(const IndexRange& range);

        private:
            VmaAllocationCreateInfo m_pageCreateInfo{};
            VkMemoryRequirements m_memoryRequirements{};

            const VulkanDriver* m_driver = nullptr;
            const VkBuffer m_targetBuffer = VK_NULL_HANDLE;
            Page* m_firstPage = nullptr;
            PK::Utilities::FixedPool<Page, 1024> m_pages;
            PK::Utilities::RangeTable<1024> m_residency;
            std::string m_name;
    };
}