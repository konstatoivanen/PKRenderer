#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanCommandBufferPool.h"

namespace PK
{
    VulkanCommandBufferPool::VulkanCommandBufferPool(VkDevice device, const VulkanServiceContext& services, uint32_t queueFamily) :
        m_queueFamily(queueFamily),
        m_device(device),
        m_primaryRenderState(services)
    {
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queueFamily;

        VK_ASSERT_RESULT(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool));

        for (auto i = 0u; i < MAX_COMMANDBUFFERS; ++i)
        {
            VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            VK_ASSERT_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_fences[i]));
        }
    }

    VulkanCommandBufferPool::~VulkanCommandBufferPool()
    {
        Prune(true);
        vkDestroyCommandPool(m_device, m_pool, nullptr);

        for (auto& fence : m_fences)
        {
            vkDestroyFence(m_device, fence, nullptr);
        }
    }

    VulkanCommandBuffer* VulkanCommandBufferPool::GetCurrent()
    {
        if (m_current != nullptr)
        {
            return m_current;
        }

        // Allocate all command buffers that have been completed.
        // Doesnt assign references to wrappers (cache warmup essentially).
        {
            VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            allocateInfo.commandPool = m_pool;
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 0u;

            for (auto& commandBuffer : m_commandBuffers)
            {
                allocateInfo.commandBufferCount += (uint32_t)(commandBuffer == VK_NULL_HANDLE);
            }

            if (allocateInfo.commandBufferCount > 0)
            {
                VkCommandBuffer buffers[MAX_COMMANDBUFFERS];
                VK_ASSERT_RESULT(vkAllocateCommandBuffers(m_device, &allocateInfo, buffers));

                for (uint32_t i = 0u, j = 0u; i < MAX_COMMANDBUFFERS; ++i)
                {
                    if (m_commandBuffers[i] == VK_NULL_HANDLE)
                    {
                        m_commandBuffers[i] = buffers[j++];
                    }
                }
            }
        }

        for (auto& commandBuffer : m_wrappers)
        {
            if (!commandBuffer.IsActive())
            {
                m_current = &commandBuffer;
                break;
            }
        }

        PK_THROW_ASSERT(m_current, "No available command buffers!");

        auto commandBuffer = m_commandBuffers[(int64_t)(m_current - &m_wrappers[0])];
        auto fence = m_fences[(int64_t)(m_current - &m_wrappers[0])];
        m_current->BeginRecord(commandBuffer, fence, m_queueFamily, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &m_primaryRenderState);

        return m_current;
    }

    VulkanCommandBuffer* VulkanCommandBufferPool::EndCurrent()
    {
        if (m_current == nullptr)
        {
            return nullptr;
        }

        auto cmd = m_current;
        m_current->EndRecord();
        m_current = nullptr;
        return cmd;
    }

    void VulkanCommandBufferPool::Prune(bool all)
    {
        VkFence fences[MAX_COMMANDBUFFERS];
        uint32_t count = 0;

        for (auto& wrapper : m_wrappers)
        {
            if (m_current != &wrapper)
            {
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
        }

        if (count > 0)
        {
            VK_ASSERT_RESULT(vkWaitForFences(m_device, count, fences, all ? VK_TRUE : VK_FALSE, UINT64_MAX));
        }

        for (auto& wrapper : m_wrappers)
        {
            if (wrapper.IsActive() && vkGetFenceStatus(m_device, wrapper.GetFence()) == VK_SUCCESS)
            {
                m_commandBuffers[(int64_t)(&wrapper - &m_wrappers[0])] = VK_NULL_HANDLE;
                vkFreeCommandBuffers(m_device, m_pool, 1, &wrapper.GetCommandBuffer());
                VK_ASSERT_RESULT(vkResetFences(m_device, 1, &wrapper.GetFence()));
                wrapper.FinishExecution();
            }
        }
    }
}