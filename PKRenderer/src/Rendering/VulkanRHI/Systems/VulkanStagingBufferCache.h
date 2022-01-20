#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/Pool.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

    struct VulkanStagingBuffer : public VulkanRawBuffer
    {
        VulkanStagingBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo) : VulkanRawBuffer(device, allocator, createInfo) {}
        mutable ExecutionGate executionGate;
        mutable VkDeviceSize destinationOffset = 0ull;
        mutable VkDeviceSize desitnationRange = 0ull;
        uint64_t pruneTick = 0ull;
    };

    class VulkanStagingBufferCache : public NoCopy
    {
        public:
            VulkanStagingBufferCache(VkDevice device, VmaAllocator allocator, uint64_t pruneDelay) : 
                    m_allocator(allocator), 
                    m_device(device),
                    m_pruneDelay(pruneDelay)
            {
                m_activeBuffers.reserve(32);
                m_freeBuffers.reserve(32);
            }

            ~VulkanStagingBufferCache();
            VulkanStagingBuffer* GetBuffer(size_t size);
            void Prune();

        private:
            const VmaAllocator m_allocator;
            const VkDevice m_device;
            std::vector<VulkanStagingBuffer*> m_freeBuffers;
            std::vector<VulkanStagingBuffer*> m_activeBuffers;
            Pool<VulkanStagingBuffer, 1024> m_bufferPool;

            uint64_t m_currentPruneTick = 0ull;
            uint64_t m_pruneDelay = 0ull;
    };
}