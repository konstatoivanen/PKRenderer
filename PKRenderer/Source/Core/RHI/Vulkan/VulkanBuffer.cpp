#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "Core/CLI/Log.h"
#include "VulkanBuffer.h"

namespace PK
{
    VulkanBuffer::VulkanBuffer(VulkanDriver* driver, size_t size, BufferUsage usage, const char* name) :
        m_name(name),
        m_driver(driver),
        m_size(size),
        m_usage(usage)
    {
        const auto& queueFamilies = m_driver->queues->GetFamiliesAll();

        // All buffers are concurrent. 
        // this doesn't carry penalties on modern hardware, unlike for images which will lose DCC if concurrent.
        VmaAllocationCreateInfo allocInfo{};
        VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        createInfo.size = size;
        createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = queueFamilies.count;
        createInfo.pQueueFamilyIndices = queueFamilies.indices;
        createInfo.usage =
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

        if ((usage & BufferUsage::AccelerationStructure) != 0)  createInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        if ((usage & BufferUsage::InstanceInput) != 0)          createInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        if ((usage & BufferUsage::ShaderBindingTable) != 0)     createInfo.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;

        if ((usage & BufferUsage::TypeBits) == BufferUsage::Vram)    createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARRead) createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamRead) createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if ((usage & BufferUsage::TypeBits) == BufferUsage::Vram)     allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARWrite) allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARRead)  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamWrite) allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamRead)  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

        if ((usage & BufferUsage::TypeBits) == BufferUsage::Vram)     allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARWrite) allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARRead)  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamWrite) allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamRead)  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARWrite) allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARRead)  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamWrite) allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamRead)  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        PK_FATAL_ASSERT((usage & BufferUsage::TypeBits) != BufferUsage::None, "Buffer memory usage not specified!");

        VK_ASSERT_RESULT_CTX(vmaCreateBuffer(driver->allocator, &createInfo, &allocInfo, &m_buffer, &m_memory, nullptr), "Failed to create a buffer!");
        VulkanSetObjectDebugName(driver->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, name);

        m_defaultView.memory.address = VulkanGetBudderDeviceAddress(driver->device, m_buffer);
        m_defaultView.memory.size = m_size;
        m_defaultView.UpdateHashMemory();

        if ((m_usage & BufferUsage::TypeBits) != BufferUsage::Vram)
        {
            VmaAllocationInfo allocationInfo{};
            vmaGetAllocationInfo(driver->allocator, m_memory, &allocationInfo);
            m_mappedData = allocationInfo.pMappedData;
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        // @TODO LAST SUBMIT FENCE IS NOT SAFE TO USE WITH GRAPH BUILDER
        // there might be queued resource access that doesn't actually match with whatever command buffer this was used on after delete.
        auto fence = m_driver->graph->GetFenceRef();

        m_driver->disposer->Dispose(m_driver->device, m_buffer, [](void* c, void* v)
        {
            vkDestroyBuffer(static_cast<VkDevice>(c), static_cast<VkBuffer>(v), nullptr);
        },
        fence);

        m_driver->disposer->Dispose(m_driver->allocator, m_memory, [](void* c, void* v)
        {
            vmaFreeMemory(static_cast<VmaAllocator>(c), static_cast<VmaAllocation>(v));
        },
        fence);

        m_mappedData = nullptr;
        m_memory = nullptr;
        m_buffer = nullptr;
        m_defaultView = {};
    }

    void* VulkanBuffer::GetMappedPtr() const
    {
        PK_DEBUG_FATAL_ASSERT(m_memory, "Trying to map a buffer without dedicated memory!");
        PK_DEBUG_FATAL_ASSERT((m_usage & BufferUsage::TypeBits) != BufferUsage::Vram, "Cant map a vram only buffer");
        return m_mappedData;
    }

    VulkanResourceView VulkanBuffer::GetResourceView(const BufferIndexRange& range) const
    {
        PK_DEBUG_FATAL_ASSERT(range.offset + range.count <= GetSize(), "Trying to get a buffer resource view for a range that is outside of buffer bounds");

        if (range.offset == 0ull && (range.count == 0ull || range.count == m_size))
        {
            return m_defaultView;
        }

        VulkanResourceView view{};
        view.memory.address = m_defaultView.memory.address + range.offset;
        view.memory.size = range.count ? range.count : (m_size - range.offset);
        view.UpdateHashMemory();
        return view;
    }
}
