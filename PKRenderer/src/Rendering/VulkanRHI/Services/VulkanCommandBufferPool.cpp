#include "PrecompiledHeader.h"
#include "VulkanCommandBufferPool.h"
#include "Core/Services/Log.h"
#include "Rendering/VUlkanRHI/Utilities/VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI::Services
{
    using namespace Objects;
    using namespace PK::Utilities;

    VulkanCommandBufferPool::VulkanCommandBufferPool(const VkDevice device, const VulkanServiceContext& systems, Objects::VulkanQueue* queue) :
        m_device(device), 
        m_primaryRenderState(systems), 
        m_queue(queue)
    {
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queue->GetFamily();
        VK_ASSERT_RESULT(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool));

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

        m_current->BeginCommandBuffer();
        return m_current;
    }

    void VulkanCommandBufferPool::SubmitCurrent(VkPipelineStageFlags flags, bool waitForPrevious, VkSemaphore* outSignal)
    {
        if (m_current == nullptr)
        {
            return;
        }

        // End possibly active render pass
        m_current->EndCommandBuffer();
        VK_ASSERT_RESULT(m_queue->Submit(m_current, flags, waitForPrevious, outSignal));
        m_current = nullptr;
    }

    void VulkanCommandBufferPool::PruneStaleBuffers()
    {
        for (auto& wrapper : m_commandBuffers)
        {
            if (wrapper.IsActive() && vkWaitForFences(m_device, 1, &wrapper.fence->vulkanFence, VK_TRUE, 0) == VK_SUCCESS)
            {
                vkFreeCommandBuffers(m_device, m_pool, 1, &wrapper.commandBuffer);
                wrapper.commandBuffer = VK_NULL_HANDLE;
                wrapper.invocationIndex++;
                VK_ASSERT_RESULT(vkResetFences(m_device, 1, &wrapper.fence->vulkanFence));
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
}