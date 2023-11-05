#include "PrecompiledHeader.h"
#include "VulkanCommandBufferPool.h"
#include "Core/Services/Log.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanUtilities.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::RHI::Vulkan::Objects;

    VulkanCommandBufferPool::VulkanCommandBufferPool(VkDevice device, const VulkanServiceContext& services, uint32_t queueFamily, VkPipelineStageFlags capabilities) :
        m_device(device),
        m_primaryRenderState(services)
    {
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queueFamily;
        VK_ASSERT_RESULT(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool));

        for (auto i = 0u; i < MAX_PRIMARY_COMMANDBUFFERS; ++i)
        {
            VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            VkFence fence = VK_NULL_HANDLE;
            VK_ASSERT_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
            m_commandBuffers[i].Initialize(fence, queueFamily, capabilities);
        }
    }

    VulkanCommandBufferPool::~VulkanCommandBufferPool()
    {
        Prune(true);
        vkDestroyCommandPool(m_device, m_pool, nullptr);

        for (auto& wrapper : m_commandBuffers)
        {
            vkDestroyFence(m_device, wrapper.GetFence(), nullptr);
        }
    }

    VulkanCommandBuffer* VulkanCommandBufferPool::GetCurrent()
    {
        if (m_current != nullptr)
        {
            return m_current;
        }

        for (auto& commandBuffer : m_commandBuffers)
        {
            if (!commandBuffer.IsActive())
            {
                m_current = &commandBuffer;
                break;
            }
        }

        PK_THROW_ASSERT(m_current, "No available command buffers!");

        const int64_t index = m_current - &m_commandBuffers[0];

        AllocateBuffers();
        m_current->BeginCommandBuffer(m_nativeBuffers[index], VK_COMMAND_BUFFER_LEVEL_PRIMARY, &m_primaryRenderState);
        return m_current;
    }

    Objects::VulkanCommandBuffer* VulkanCommandBufferPool::EndCurrent()
    {
        if (m_current == nullptr)
        {
            return nullptr;
        }

        auto cmd = m_current;
        m_current->EndCommandBuffer();
        m_current = nullptr;
        return cmd;
    }

    void VulkanCommandBufferPool::Prune(bool all)
    {
        VkFence fences[MAX_PRIMARY_COMMANDBUFFERS];
        uint32_t count = 0;

        for (auto& wrapper : m_commandBuffers)
        {
            if (m_current == &wrapper)
            {
                continue;
            }

            if (wrapper.IsActive())
            {
                fences[count++] = wrapper.GetFence();
            }
            else if (!all)
            {
                // At least one command buffer has been released/completed.
                count = 0u;
                break;
            }
        }

        if (count > 0)
        {
            VK_ASSERT_RESULT(vkWaitForFences(m_device, count, fences, all ? VK_TRUE : VK_FALSE, UINT64_MAX));
        }

        for (auto& wrapper : m_commandBuffers)
        {
            if (wrapper.IsActive() && vkWaitForFences(m_device, 1, &wrapper.GetFence(), VK_TRUE, 0) == VK_SUCCESS)
            {
                m_nativeBuffers[(int64_t)(&wrapper - &m_commandBuffers[0])] = VK_NULL_HANDLE;
                vkFreeCommandBuffers(m_device, m_pool, 1, &wrapper.GetNative());
                VK_ASSERT_RESULT(vkResetFences(m_device, 1, &wrapper.GetFence()));
                wrapper.Release();
            }
        }
    }

    void VulkanCommandBufferPool::AllocateBuffers()
    {
        VkCommandBuffer buffers[MAX_PRIMARY_COMMANDBUFFERS];
        uint32_t count = 0u;

        for (auto i = 0u; i < MAX_PRIMARY_COMMANDBUFFERS; ++i)
        {
            if (m_nativeBuffers[i] == VK_NULL_HANDLE)
            {
                ++count;
            }
        }

        if (count == 0)
        {
            return;
        }

        VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = m_pool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = count;
        VK_ASSERT_RESULT(vkAllocateCommandBuffers(m_device, &allocateInfo, buffers));
        count = 0u;

        for (auto i = 0u; i < MAX_PRIMARY_COMMANDBUFFERS; ++i)
        {
            if (m_nativeBuffers[i] == VK_NULL_HANDLE)
            {
                m_nativeBuffers[i] = buffers[count++];
            }
        }
    }
}