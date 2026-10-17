#include "PrecompiledHeader.h"
#include "Core/Base/Reflect.h"
#include "Core/Base/Memory.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanQueue.h"

namespace PK
{
    struct QueueFindContext
    {
        VulkanQueueSetInitializer* initializer;
        const VkQueueFamilyProperties* properties;
        const bool* presentSupport;
        uint32_t* familyToQueueIndex;
        uint32_t familyCount;
    };

    static uint32_t GetQueueIndex(QueueFindContext& ctx,
        VkQueueFlags requiredFlags,
        VkQueueFlags optionalFlags,
        bool requirePresent,
        bool optionalPresent,
        bool preferSeparate)
    {
        auto selectedFamily = ~0u;
        auto heuristic = 0u;
        auto bestOrder = ~0u;

        for (auto familyIndex = 0u; familyIndex < ctx.familyCount; ++familyIndex)
        {
            const auto& properties = ctx.properties[familyIndex];
            const auto presentSupport = ctx.presentSupport[familyIndex];
            const auto existingQueueIndex = ctx.familyToQueueIndex[familyIndex];
            const auto isAllocated = existingQueueIndex != 0xFFFFFFFFu;

            if ((properties.queueFlags & requiredFlags) != requiredFlags || (!presentSupport && requirePresent))
            {
                continue;
            }

            auto score = 0u;
            score += math::bitcount(properties.queueFlags & optionalFlags);
            score += presentSupport && optionalPresent ? 2u : 1u;
            score += !isAllocated && preferSeparate ? 1u : 0u;

            const auto order = isAllocated ? existingQueueIndex : (ctx.familyCount + familyIndex);

            if (score > heuristic || (score == heuristic && order < bestOrder))
            {
                heuristic = score;
                bestOrder = order;
                selectedFamily = familyIndex;
            }
        }

        PK_FATAL_ASSERT(selectedFamily != 0xFFFFFFFFu, "Failed to find queue matching parameters!");

        if (ctx.familyToQueueIndex[selectedFamily] == 0xFFFFFFFFu)
        {
            const auto index = ctx.initializer->queueCount++;
            ctx.familyToQueueIndex[selectedFamily] = index;
            ctx.initializer->queueFamilies[index] = selectedFamily;
            ctx.initializer->familyProperties[index] = ctx.properties[selectedFamily];
            ctx.initializer->createInfos[index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            ctx.initializer->createInfos[index].pNext = nullptr;
            ctx.initializer->createInfos[index].flags = 0u;
            ctx.initializer->createInfos[index].queueFamilyIndex = selectedFamily;
            ctx.initializer->createInfos[index].queueCount = 1u;
            ctx.initializer->createInfos[index].pQueuePriorities = ctx.initializer->priorities;
        }

        return ctx.familyToQueueIndex[selectedFamily];
    }

    VulkanQueueSetInitializer::VulkanQueueSetInitializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        auto* queueFamilyProperties = PK_STACK_ALLOC(VkQueueFamilyProperties, queueFamilyCount);
        auto* presentSupport = PK_STACK_ALLOC(bool, queueFamilyCount);
        auto* familyToQueueIndex = PK_STACK_ALLOC(uint32_t, queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);

        {
            PK_LOG_INFO_SCOPE("VulkanQueueSet.Initializer: Found '%u' Physical Device Queue Families:", queueFamilyCount);

            for (auto i = 0u; i < queueFamilyCount; ++i)
            {
                presentSupport[i] = VulkanIsPresentSupported(physicalDevice, i, surface);
                familyToQueueIndex[i] = 0xFFFFFFFFu;
                auto& props = queueFamilyProperties[i];
                PK_LOG_INFO("Family: %i, NumQueues: %i, Flags: %s", i, props.queueCount, VulkanStr_VkQueueFlags(props.queueFlags));
            }

            PK_LOG_NEWLINE();
        }

        QueueFindContext context;
        context.initializer = this;
        context.properties = queueFamilyProperties;
        context.presentSupport = presentSupport;
        context.familyToQueueIndex = familyToQueueIndex;
        context.familyCount = queueFamilyCount;

        const auto maskTransfer = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
        const auto maskGraphics = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        const auto maskCompute = VK_QUEUE_COMPUTE_BIT;

        typeIndices[(uint32_t)QueueType::Graphics] = GetQueueIndex(context, maskGraphics, maskTransfer, false, true, true);
        typeIndices[(uint32_t)QueueType::Present] = GetQueueIndex(context, maskGraphics, maskTransfer, true, false, false);
        typeIndices[(uint32_t)QueueType::Compute] = GetQueueIndex(context, maskCompute, maskTransfer, false, false, true);
        typeIndices[(uint32_t)QueueType::Transfer] = GetQueueIndex(context, maskTransfer, 0u, false, false, true);

        for (auto i = (uint32_t)QueueType::EnumCount; i > 0u; --i)
        {
            names[typeIndices[i - 1u]] = ReflectEnum<QueueType>::Names[i - 1u];
        }

        {
            PK_LOG_INFO_SCOPE("VulkanQueueSet.Initializer: Selected '%u' Queues From '%u' Physical Device Queue Families:", queueCount, queueFamilyCount);

            for (auto i = 0u; i < queueCount; ++i)
            {
                PK_LOG_INFO("Family: %u", queueFamilies[i]);
            }

            PK_LOG_NEWLINE();
        }

        selectedAll.count = queueCount;
        memcpy(selectedAll.indices, queueFamilies, sizeof(queueFamilies));

        selectedRender.count = 0u;
        uint32_t renderFamilyMask = 0u;

        for (auto i = 0u; i < (uint32_t)QueueType::EnumCount; ++i)
        {
            if ((QueueType)i != QueueType::Transfer)
            {
                const auto family = queueFamilies[typeIndices[i]];
                if ((renderFamilyMask & (1u << family)) == 0u)
                {
                    renderFamilyMask |= 1u << family;
                    selectedRender.indices[selectedRender.count++] = family;
                }
            }
        }
    }

    VulkanQueue::VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, uint32_t queueIndex, VkDeviceAddress descriptorHeapAddress, const char* name) :
        m_device(device),
        m_family(queueFamily),
        m_queueIndex(queueIndex),
        m_queueFlags(flags),
        m_accessFlags(VulkanEnumConvert::GetQueueFlagsAccessCapabilities(flags)),
        m_stageFlags(VulkanEnumConvert::GetQueueFlagsStageCapabilities(flags)),
        m_descriptorHeapAddress(descriptorHeapAddress)
    {
        vkGetDeviceQueue(device, m_family, m_queueIndex, &m_queue);
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_QUEUE, (uint64_t)m_queue, FixedString32("PK_Queue_%s", name).c_str());

        VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        commandPoolCreateInfo.queueFamilyIndex = queueFamily;
        VK_ASSERT_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &m_commandPool));
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_commandPool, FixedString32("PK_Cmd_Pool_%s", name).c_str());

        VkSemaphoreTypeCreateInfo timelineCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = 0ull;

        VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,};
        semaphoreCreateInfo.pNext = &timelineCreateInfo;
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_signal.semaphore);
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_signal.semaphore, FixedString32("PK_Timeline_Semaphore_%s", name).c_str());

        VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = m_commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = PK_VK_MAX_COMMAND_BUFFERS;
        VK_ASSERT_RESULT(vkAllocateCommandBuffers(device, &allocateInfo, m_commandBuffers));
    }

    VulkanQueue::~VulkanQueue()
    {
        WaitCommandBuffers(true);
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroySemaphore(m_device, m_signal.semaphore, nullptr);
    }


    FenceRef VulkanQueue::GetFenceRef(uint32_t timelineOffset) const
    {
        return FenceRef(this, [](const void* ctx, uint64_t userdata, uint64_t timeout)
            {
                auto queue = static_cast<const VulkanQueue*>(ctx);
                VkSemaphoreWaitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
                waitInfo.pNext = nullptr;
                waitInfo.flags = VK_SEMAPHORE_WAIT_ANY_BIT;
                waitInfo.semaphoreCount = 1u;
                waitInfo.pSemaphores = &queue->m_signal.semaphore;
                waitInfo.pValues = &userdata;
                auto result = vkWaitSemaphores(queue->m_device, &waitInfo, timeout);

                if (result != VK_SUCCESS && result != VK_TIMEOUT)
                {
                    VK_THROW_RESULT(result);
                }

                return result == VK_SUCCESS;
            },
            m_signal.counter + timelineOffset);
    }

    VulkanCommandBuffer* VulkanQueue::GetCommandBuffer()
    {
        if (m_currentWrapper != nullptr)
        {
            return m_currentWrapper;
        }

        for (auto i = 0u; i < PK_VK_MAX_COMMAND_BUFFERS; ++i)
        {
            if (!m_commandWrappers[i].IsExecuting())
            {
                m_currentWrapper = &m_commandWrappers[i];
                m_currentWrapper->QueueInitialize(m_commandBuffers[i], m_queueFlags, m_stageFlags, m_accessFlags, m_descriptorHeapAddress);
                return m_currentWrapper;
            }
        }

        PK_FATAL_ERROR("No available command buffers!");
        return nullptr;
    }

    void VulkanQueue::Submit(VkSemaphore inSignal)
    {
        // No commands in command buffer. dont submit empty command buffers.
        if (!m_currentWrapper || !m_currentWrapper->QueueSubmit(m_signal.counter + 1ull))
        {
            return;
        }

        VkCommandBuffer commandBuffer = *m_currentWrapper;
        m_currentWrapper = nullptr;
        m_signal.counter++;

        VkSemaphoreSubmitInfo waits[MAX_DEPENDENCIES]{};
        VkSemaphoreSubmitInfo signals[2]{};
        auto waitCount = 0u;
        auto signalCount = inSignal != VK_NULL_HANDLE ? 2u : 1u;

        for (auto& timeline : m_waits)
        {
            if (timeline.semaphore != VK_NULL_HANDLE)
            {
                waits[waitCount].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                waits[waitCount].semaphore = timeline.semaphore;
                waits[waitCount].value = timeline.counter;
                waits[waitCount].stageMask = timeline.stageFlags & m_stageFlags;
                waitCount++;
            }
        }

        signals[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signals[0].semaphore = m_signal.semaphore;
        signals[0].value = m_signal.counter;
        signals[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        // binary semaphore signal for external syncs
        if (inSignal != VK_NULL_HANDLE)
        {
            signals[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signals[1].semaphore = inSignal;
            signals[1].value = 0; 
            signals[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        }

        memset(m_waits, 0, sizeof(m_waits));

        VkCommandBufferSubmitInfo cmdInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
        cmdInfo.commandBuffer = commandBuffer;
        cmdInfo.deviceMask = 0;

        VkSubmitInfo2 submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
        submitInfo.waitSemaphoreInfoCount = waitCount;
        submitInfo.pWaitSemaphoreInfos = waits;
        submitInfo.signalSemaphoreInfoCount = signalCount;
        submitInfo.pSignalSemaphoreInfos = signals;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cmdInfo;
        VK_ASSERT_RESULT(vkQueueSubmit2(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
    }

    void VulkanQueue::Present(VkSwapchainKHR swapchain, uint32_t imageIndex, uint64_t presentId, VkPresentModeKHR mode, VkSemaphore waitSignal)
    {
        VkSwapchainPresentModeInfoKHR dynamicPresentModeInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_KHR };
        dynamicPresentModeInfo.swapchainCount = 1u;
        dynamicPresentModeInfo.pPresentModes = &mode;

        VkPresentIdKHR presentIdKHR{ VK_STRUCTURE_TYPE_PRESENT_ID_KHR };
        presentIdKHR.pNext = &dynamicPresentModeInfo;
        presentIdKHR.swapchainCount = 1u;
        presentIdKHR.pPresentIds = &presentId;

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.pNext = &presentIdKHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSignal;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; 
        VK_ASSERT_RESULT(vkQueuePresentKHR(m_queue, &presentInfo));
    }

    void VulkanQueue::AddWait(VkSemaphore semaphore, VkPipelineStageFlags2 flags, uint64_t counter)
    {
        const auto stageFlags = flags ? flags : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        
        for (auto& wait : m_waits)
        {
            if (wait.semaphore == semaphore)
            {
                wait.counter = math::max(wait.counter, counter);
                wait.stageFlags |= stageFlags;
                return;
            }

            if (wait.semaphore == VK_NULL_HANDLE)
            {
                wait.semaphore = semaphore;
                wait.counter = counter;
                wait.stageFlags = stageFlags;
                return;
            }
        }
    }

    void VulkanQueue::AddWait(VulkanQueue* other, VkPipelineStageFlags2 flags)
    {
        if (other != this && other->m_signal.counter > 0ull)
        {
            AddWait(other->m_signal.semaphore, flags, other->m_signal.counter);
        }
    }

    void VulkanQueue::WaitCommandBuffers(bool waitAll)
    {
        if (waitAll)
        {
            vkQueueWaitIdle(m_queue);
        }

        uint64_t counterValue = 0ull;
        VK_ASSERT_RESULT(vkGetSemaphoreCounterValue(m_device, m_signal.semaphore, &counterValue));

        for (auto& wrapper : m_commandWrappers)
        {
            wrapper.QueueFinalize(counterValue);
        }
    }


    VulkanQueueSet::VulkanQueueSet(const VkDevice device, VkDeviceAddress descriptorHeapAddress, const VulkanQueueSetInitializer& initializer) :
        m_device(device),
        m_queueCount(initializer.queueCount),
        m_familiesAll(initializer.selectedAll),
        m_familiesRender(initializer.selectedRender)
    {
        memcpy(m_indices, initializer.typeIndices, sizeof(m_indices));

        for (auto i = 0u; i < initializer.queueCount; ++i)
        {
            m_queues[i] = CreateUnique<VulkanQueue>(device, initializer.familyProperties[i].queueFlags, initializer.queueFamilies[i], 0, descriptorHeapAddress, initializer.names[i]);
        }
    }

    FenceRef VulkanQueueSet::GetFenceRef() const
    {
        const auto& snapshotCurrent = m_fenceSnapshots[m_fenceCounter % MAX_FENCE_SNAPSHOTS];
        VkSemaphore semaphores[(uint32_t)QueueType::EnumCount]{};
        uint64_t counters[(uint32_t)QueueType::EnumCount]{};
        auto isDirty = m_fenceCounter == 0ull;

        for (auto i = 0u; i < m_queueCount; ++i)
        {
            semaphores[i] = m_queues[i]->m_signal.semaphore;
            counters[i] = m_queues[i]->m_signal.counter;
            isDirty |= (counters[i] != snapshotCurrent.values[i]);
        }

        if (isDirty)
        {
            const auto index = ++m_fenceCounter;
            auto& snapshotNew = m_fenceSnapshots[index % MAX_FENCE_SNAPSHOTS];

            if (snapshotNew.index != 0ull)
            {
                VkSemaphoreWaitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
                waitInfo.pNext = nullptr;
                waitInfo.flags = 0u;
                waitInfo.semaphoreCount = m_queueCount;
                waitInfo.pSemaphores = semaphores;
                waitInfo.pValues = snapshotNew.values;
                VK_ASSERT_RESULT(vkWaitSemaphores(m_device, &waitInfo, UINT64_MAX));
            }

            snapshotNew.index = index;
            memcpy(snapshotNew.values, counters, sizeof(counters));
        }

        return FenceRef(this, [](const void* ctx, uint64_t userdata, uint64_t timeout)
        {
            const auto queues = static_cast<const VulkanQueueSet*>(ctx);
            const auto& snapshot = queues->m_fenceSnapshots[userdata % MAX_FENCE_SNAPSHOTS];
            VkSemaphore semaphores[(uint32_t)QueueType::EnumCount]{};

            if (snapshot.index != userdata)
            {
                return snapshot.index > userdata;
            }

            for (auto i = 0u; i < queues->m_queueCount; ++i)
            {
                semaphores[i] = queues->m_queues[i]->m_signal.semaphore;
            }

            VkSemaphoreWaitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
            waitInfo.pNext = nullptr;
            waitInfo.flags = 0u;
            waitInfo.semaphoreCount = queues->m_queueCount;
            waitInfo.pSemaphores = semaphores;
            waitInfo.pValues = snapshot.values;
            auto result = vkWaitSemaphores(queues->m_device, &waitInfo, timeout);

            if (result != VK_SUCCESS && result != VK_TIMEOUT)
            {
                VK_THROW_RESULT(result);
            }

            return result == VK_SUCCESS;
        },
        m_fenceCounter);
    }

    void VulkanQueueSet::Prune()
    {
        for (auto& queue : m_queues)
        {
            if (queue != nullptr)
            {
                queue->WaitCommandBuffers(false);
            }
        }
    }
}
