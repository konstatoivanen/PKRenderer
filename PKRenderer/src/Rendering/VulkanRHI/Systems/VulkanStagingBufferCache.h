#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

    struct VulkanStagingBuffer : public VulkanRawBuffer
    {
        VulkanStagingBuffer(VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo) : VulkanRawBuffer(allocator, createInfo) {}
        mutable VulkanExecutionGate executionGate;
        mutable VkDeviceSize destinationOffset = 0ull;
        mutable VkDeviceSize desitnationRange = 0ull;
        uint64_t pruneTick = 0ull;
    };

    class VulkanStagingBufferCache : public PK::Core::NoCopy
    {
        public:
            VulkanStagingBufferCache(VmaAllocator allocator, uint64_t pruneDelay) : m_allocator(allocator), m_pruneDelay(pruneDelay) {}
            ~VulkanStagingBufferCache();

            const VulkanStagingBuffer* GetBuffer(size_t size);
            void Prune();

        private:
            const VmaAllocator m_allocator;

            std::multimap<VkDeviceSize, VulkanStagingBuffer*> m_freeBuffers;
            std::unordered_set<VulkanStagingBuffer*> m_activeBuffers;

            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}