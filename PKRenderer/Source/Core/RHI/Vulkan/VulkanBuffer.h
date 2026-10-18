#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanBuffer : public RHIBuffer
    {
        VulkanBuffer(struct VulkanDriver* driver, size_t size, BufferUsage usage, const char* name);
        ~VulkanBuffer();
        
        size_t GetSize() const final { return m_size; }
        BufferUsage GetUsage() const final { return m_usage; }
        const char* GetDebugName() const final { return m_name.c_str(); }
        uint64_t GetDeviceAddress() const final { return m_defaultView.memory.address; }
        const void* GetNativeView() const final { return &m_defaultView; }
        void* GetMappedPtr() const final;

        VulkanResourceView GetResourceView(const BufferIndexRange& range) const;

    private:
        const FixedString128 m_name;
        const VulkanDriver* m_driver;
        const VkDeviceSize m_size;
        const BufferUsage m_usage;
        void* m_mappedData;
        VmaAllocation m_memory;
        VkBuffer m_buffer;
        VulkanResourceView m_defaultView{};
    };
}
