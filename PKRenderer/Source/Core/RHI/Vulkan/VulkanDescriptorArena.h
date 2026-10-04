#pragma once
#include "Core/Base/NoCopy.h"
#include "Core/Base/Containers/Mask.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDescriptorArena : public NoCopy
    {
        static constexpr uint32_t MAX_RANGES = 64u;
    
        struct DescriptorRange
        {
            VkDeviceSize offset;
            VkDeviceSize size;
        };
    
        VulkanDescriptorArena(const VulkanDriver* driver, const VulkanPhysicalDeviceProperties& properties, uint32_t size);
        ~VulkanDescriptorArena();
    
        VkDeviceSize AllocateDescriptorSet(const VulkanDescriptorSetLayout* layout, const VulkanDescriptorBinding* bindings, const uint32_t bindingCount);
        void BeginRange();
        uint64_t EndRange();
        void FreeRange(uint64_t rangeIndex);
        
        inline bool IsValid() const { return m_size; }
        inline VkDeviceAddress GetDeviceAddress() const { return m_deviceAddress; }
    
    private:
        const VkDevice m_device;
        const VmaAllocator m_allocator;
        const VkDeviceSize m_size;
        VmaAllocation m_memory;
        VkDeviceAddress m_deviceAddress;
        VkBuffer m_buffer;
        void* m_mappedData;

        VkDeviceSize m_descriptorBufferOffsetAlignment = 64;
        VkDeviceSize m_descriptorSizes[(uint32_t)ShaderResourceType::EnumCount]{};

        FixedMask<MAX_RANGES> m_rangeMask;
        DescriptorRange m_ranges[MAX_RANGES];

        uint64_t m_rangeHead = 0ull;
        uint64_t m_bufferHead = 0ull;
        uint64_t m_bufferFlushHead = 0ull;
        uint64_t m_rangeFlushHead = 0ull;
        bool m_inRange = false;

    };
}