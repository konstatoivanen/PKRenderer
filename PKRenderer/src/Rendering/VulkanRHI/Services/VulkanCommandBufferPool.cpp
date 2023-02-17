#include "PrecompiledHeader.h"
#include "VulkanCommandBufferPool.h"
#include "Core/Services/Log.h"
#include "Rendering/VUlkanRHI/Utilities/VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI::Services
{
    using namespace Objects;
    using namespace PK::Utilities;

    VulkanCommandBufferPool::VulkanCommandBufferPool(VkDevice device, const VulkanServiceContext& services, uint32_t queueFamily) :
        m_device(device), 
        m_primaryRenderState(services)
    {
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queueFamily;
        VK_ASSERT_RESULT(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool));

        for (auto& wrapper : m_commandBuffers)
        {
            VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            VK_ASSERT_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &wrapper.GetFence()));
        }
    }

    VulkanCommandBufferPool::~VulkanCommandBufferPool()
    {
        WaitForCompletion(true);
        PruneStaleBuffers();
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

        WaitForCompletion(false);
        PruneStaleBuffers();

        for (auto& commandBuffer : m_commandBuffers)
        {
            if (!commandBuffer.IsActive())
            {
                m_current = &commandBuffer;
                break;
            }
        }

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = m_pool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        VK_ASSERT_RESULT(vkAllocateCommandBuffers(m_device, &allocateInfo, &commandBuffer));

        m_current->BeginCommandBuffer(commandBuffer, allocateInfo.level, &m_primaryRenderState);
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

    void VulkanCommandBufferPool::PruneStaleBuffers()
    {
        for (auto& wrapper : m_commandBuffers)
        {
            if (wrapper.IsActive() && vkWaitForFences(m_device, 1, &wrapper.GetFence(), VK_TRUE, 0) == VK_SUCCESS)
            {
                vkFreeCommandBuffers(m_device, m_pool, 1, &wrapper.GetNative());
                wrapper.Release();
                VK_ASSERT_RESULT(vkResetFences(m_device, 1, &wrapper.GetFence()));
            }
        }
    }

    void VulkanCommandBufferPool::WaitForCompletion(bool all)
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
                return;
            }
        }

        if (count > 0)
        {
            VK_ASSERT_RESULT(vkWaitForFences(m_device, count, fences, all ? VK_TRUE : VK_FALSE, UINT64_MAX));
        }
    }
}