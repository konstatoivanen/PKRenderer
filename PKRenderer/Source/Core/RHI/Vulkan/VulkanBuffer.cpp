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
        const auto& queueFamilies = m_driver->queues->GetSelectedFamilies();

        // All buffers are concurrent. 
        // this doesn't carry penalties on modern hardware, unlike for images which will lose DCC if concurrent.
        VmaAllocationCreateInfo allocInfo{};
        VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        createInfo.size = size;
        createInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = queueFamilies.count;
        createInfo.pQueueFamilyIndices = queueFamilies.indices;

        if ((usage & BufferUsage::TransferSrc) != 0)            createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if ((usage & BufferUsage::Vertex) != 0)                 createInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        if ((usage & BufferUsage::Index) != 0)                  createInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        if ((usage & BufferUsage::Constant) != 0)               createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if ((usage & BufferUsage::Storage) != 0)                createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if ((usage & BufferUsage::Indirect) != 0)               createInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
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

        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARWrite) allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamWrite) allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::BARRead)  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        if ((usage & BufferUsage::TypeBits) == BufferUsage::RamRead)  allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        PK_FATAL_ASSERT((usage & BufferUsage::TypeBits) != BufferUsage::None, "Buffer memory usage not specified!");

        VK_ASSERT_RESULT_CTX(vmaCreateBuffer(driver->allocator, &createInfo, &allocInfo, &m_buffer, &m_memory, nullptr), "Failed to create a buffer!");
        VulkanSetObjectDebugName(driver->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, name);

        VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        addressInfo.buffer = m_buffer;
        m_deviceAddress = vkGetBufferDeviceAddress(driver->device, &addressInfo);

        if ((m_usage & BufferUsage::TypeBits) != BufferUsage::Vram)
        {
            VmaAllocationInfo allocationInfo{};
            vmaGetAllocationInfo(driver->allocator, m_memory, &allocationInfo);
            m_mappedData = allocationInfo.pMappedData;
        }

        // only vram resident buffers can be bound.
        if ((m_usage & BufferUsage::TypeBits) == BufferUsage::Vram)
        {
            GetNativeView({ 0, GetSize() });
            m_defaultView = m_firstView;
        }
    }

    VulkanBuffer::~VulkanBuffer()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        for (auto view : m_firstView)
        {
            m_driver->DisposePooled(view, fence);
        }

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
        m_deviceAddress = 0ull;
        m_firstView = nullptr;
        m_defaultView = nullptr;
    }

    void* VulkanBuffer::BeginMap(size_t offset, size_t readSize) const
    {
        PK_DEBUG_FATAL_ASSERT(m_memory, "Trying to map a buffer without dedicated memory!");
        PK_DEBUG_FATAL_ASSERT((m_usage & BufferUsage::TypeBits) != BufferUsage::Vram, "Cant map a vram only buffer");
        PK_DEBUG_FATAL_ASSERT((offset + readSize) <= GetSize(), "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", offset + readSize, GetSize());

        if (readSize > 0ull)
        {
            vmaInvalidateAllocation(m_driver->allocator, m_memory, offset, readSize);
        }

        return static_cast<char*>(m_mappedData) + offset;
    }

    void VulkanBuffer::EndMap(size_t offset, size_t writeSize) const
    {
        PK_DEBUG_FATAL_ASSERT(m_memory, "Trying to umap a buffer without dedicated memory!");

        if (writeSize > 0ull)
        {
            vmaFlushAllocation(m_driver->allocator, m_memory, offset, writeSize);
        }
    }

    const void* VulkanBuffer::GetNativeView(const BufferIndexRange& range)
    {
        PK_DEBUG_FATAL_ASSERT(range.offset + range.count <= GetSize() && range.count != VK_WHOLE_SIZE, "Trying to get a buffer bind handle for a range that it outside of buffer bounds");

        if (m_firstView.FindAndSwapFirst(range))
        {
            return m_firstView;
        }

        auto view = m_driver->CreatePooled<VulkanBufferView>();
        view->buffer.buffer = m_buffer;
        view->buffer.deviceAddress = m_deviceAddress;
        view->buffer.range = range.count;
        view->buffer.offset = range.offset;
        view->isConcurrent = true;
        m_firstView.Insert(view, range);
        return m_firstView;
    }
}
