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
        uint64_t GetDeviceAddress() const final { return m_deviceAddress; }
        const void* GetNativeView() const final { return m_defaultView; }
        const void* GetNativeView(const BufferIndexRange& range);

        void* BeginMap(size_t offset, size_t readsize) const final;
        void EndMap(size_t offset, size_t writeSize) const final;

    private:
        const FixedString128 m_name;
        const VulkanDriver* m_driver;
        const VkDeviceSize m_size;
        const BufferUsage m_usage;
        void* m_mappedData;
        VmaAllocation m_memory;
        VkBuffer m_buffer;
        VkDeviceAddress m_deviceAddress;
        
        VulkanBufferView* m_defaultView = nullptr;
        LinkedList<VulkanBufferView, BufferIndexRange> m_firstView = nullptr;
    };
}
