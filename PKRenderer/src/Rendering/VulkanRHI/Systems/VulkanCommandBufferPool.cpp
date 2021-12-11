#include "PrecompiledHeader.h"
#include "VulkanCommandBufferPool.h"
#include "Rendering/VUlkanRHI/Utilities/VulkanUtilities.h"
#include "Utilities/Log.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace Objects;

    VulkanCommandBufferPool::VulkanCommandBufferPool(const VkDevice device, const VulkanSystemContext& systems, uint32_t queueFamilyIndex) : m_device(device), m_primaryRenderState(systems)
    {
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        VK_ASSERT_RESULT(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool));
        vkGetDeviceQueue(m_device, queueFamilyIndex, 0, &m_queue);

        for (auto& semaphore : m_renderingFinishedSignals) 
        {
            semaphore = CreateRef<VulkanSemaphore>(m_device);
        }

        for (uint32_t index = 0; index < MAX_PRIMARY_COMMANDBUFFERS; ++index) 
        {
            m_commandBuffers[index].fence = CreateRef<VulkanFence>(m_device, false);
            m_commandBuffers[index].renderState = &m_primaryRenderState;
        }
    }

    VulkanCommandBufferPool::~VulkanCommandBufferPool()
    {
        WaitForCompletion(true);
        PruneStaleBuffers();
        vkDestroyCommandPool(m_device, m_pool, nullptr);
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

        VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_pool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        VK_ASSERT_RESULT(vkAllocateCommandBuffers(m_device, &allocateInfo, &m_current->commandBuffer));

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_ASSERT_RESULT(vkBeginCommandBuffer(m_current->commandBuffer, &beginInfo));

        m_current->renderState->Reset();

        return m_current;
    }

    void VulkanCommandBufferPool::SubmitCurrent(VkPipelineStageFlags waitFlag, const VulkanSemaphore* waitSignal)
    {
        if (m_current == nullptr)
        {
            return;
        }

        const int64_t index = m_current - &m_commandBuffers[0];
        auto& renderingFinished = m_renderingFinishedSignals[index];

        VK_ASSERT_RESULT(vkEndCommandBuffer(m_current->commandBuffer));

        VkSemaphore signals[2] = 
        {
            VK_NULL_HANDLE,
            VK_NULL_HANDLE,
        };

        VkPipelineStageFlags waitFlags[2] =
        {
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            waitFlag
        };

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = signals;
        submitInfo.pWaitDstStageMask = waitFlags;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_current->commandBuffer;
        submitInfo.signalSemaphoreCount = 1u;
        submitInfo.pSignalSemaphores = &renderingFinished->vulkanSemaphore;

        if (m_renderingFinishedSignal) 
        {
            signals[submitInfo.waitSemaphoreCount++] = m_renderingFinishedSignal->vulkanSemaphore;
        }

        if (waitSignal)
        {
            signals[submitInfo.waitSemaphoreCount++] = waitSignal->vulkanSemaphore;
        }

        VK_ASSERT_RESULT_CTX(vkQueueSubmit(m_queue, 1, &submitInfo, m_current->fence->vulkanFence), "Failed to submit command buffer!");

        m_renderingFinishedSignal = renderingFinished.get();
        m_current = nullptr;
    }

    void VulkanCommandBufferPool::PruneStaleBuffers()
    {
        for (auto& wrapper : m_commandBuffers)
        {
            if (wrapper.IsActive())
            {
                if (vkWaitForFences(m_device, 1, &wrapper.fence->vulkanFence, VK_TRUE, 0) == VK_SUCCESS)
                {
                    vkFreeCommandBuffers(m_device, m_pool, 1, &wrapper.commandBuffer);
                    wrapper.commandBuffer = VK_NULL_HANDLE;
                    wrapper.invocationIndex++;
                    VK_ASSERT_RESULT(vkResetFences(m_device, 1, &wrapper.fence->vulkanFence));
                }
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
                fences[count++] = wrapper.fence->vulkanFence;
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

    VulkanSemaphore* VulkanCommandBufferPool::AcquireRenderingFinishedSignal()
    {
        PK_THROW_ASSERT(m_renderingFinishedSignal != nullptr, "Trying to acquire a null rendering finished signal!");
        auto* semaphore = m_renderingFinishedSignal;
        m_renderingFinishedSignal = nullptr;
        return semaphore;
    }

}