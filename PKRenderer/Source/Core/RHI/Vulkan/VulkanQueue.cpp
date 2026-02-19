#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "VulkanQueue.h"

namespace PK
{
    struct QueueFindContext
    {
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHR surface;
        const VkQueueFamilyProperties* properties;
        VkDeviceQueueCreateInfo* createInfos;
        uint32_t* selectedIndices;
        uint32_t familyCount;
        uint32_t selectedCount;
    };

    static uint32_t GetQueueIndex(QueueFindContext& ctx,
        VkQueueFlags requiredFlags,
        VkQueueFlags optionalFlags,
        bool requirePresent,
        bool optionalPresent,
        bool preferSeparate)
    {
        auto selectedFamily = 0xFFFFFFFF;
        auto existingIndex = 0xFFFFFFFF;
        auto heuristic = 0u;

        for (auto queueIndex = 0u; queueIndex < ctx.selectedCount; ++queueIndex)
        {
            const auto familyIndex = ctx.selectedIndices[queueIndex];
            const auto& properties = ctx.properties[familyIndex];
            auto presentSupport = VulkanIsPresentSupported(ctx.physicalDevice, familyIndex, ctx.surface);

            if ((properties.queueFlags & requiredFlags) != requiredFlags || (!presentSupport && requirePresent))
            {
                continue;
            }

            auto value = Math::CountBits(properties.queueFlags & optionalFlags) + (presentSupport && optionalPresent ? 2 : 1);

            if (value > heuristic)
            {
                heuristic = value;
                selectedFamily = familyIndex;
                existingIndex = queueIndex;
            }
        }

        for (auto familyIndex = 0u; familyIndex < ctx.familyCount; ++familyIndex)
        {
            auto& properties = ctx.properties[familyIndex];
            auto presentSupport = VulkanIsPresentSupported(ctx.physicalDevice, familyIndex, ctx.surface);

            if ((properties.queueFlags & requiredFlags) != requiredFlags || ctx.createInfos[familyIndex].queueCount > 0u || (!presentSupport && requirePresent))
            {
                continue;
            }

            auto value = Math::CountBits(properties.queueFlags & optionalFlags) + (presentSupport && optionalPresent ? 2 : 1);

            if (preferSeparate)
            {
                value++;
            }

            if (value > heuristic)
            {
                heuristic = value;
                selectedFamily = familyIndex;
                existingIndex = 0xFFFFFFFF;
            }
        }

        PK_THROW_ASSERT(selectedFamily != 0xFFFFFFFF, "Failed to find queue matching parameters!");

        if (existingIndex != 0xFFFFFFFF)
        {
            return existingIndex;
        }

        ctx.createInfos[selectedFamily].queueCount = 1;
        ctx.selectedIndices[ctx.selectedCount] = selectedFamily;
        return ctx.selectedCount++;
    }

    VulkanQueueSetInitializer::VulkanQueueSetInitializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        VkQueueFamilyProperties* queueFamilyProperties = PK_STACK_ALLOC(VkQueueFamilyProperties, queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);

        VkDeviceQueueCreateInfo* queueCreateInfos = PK_STACK_ALLOC(VkDeviceQueueCreateInfo, queueFamilyCount);

        QueueFindContext context;
        context.physicalDevice = physicalDevice;
        context.surface = surface;
        context.properties = queueFamilyProperties;
        context.selectedIndices = queueFamilies;
        context.createInfos = queueCreateInfos;
        context.familyCount = queueFamilyCount;
        context.selectedCount = 0u;

        {
            PK_LOG_INFO_SCOPE("VulkanQueueSet.Initializer: Found '%u' Physical Device Queue Families:", queueFamilyCount);

            for (auto i = 0u; i < queueFamilyCount; ++i)
            {
                queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfos[i].pNext = nullptr;
                queueCreateInfos[i].flags = 0u;
                queueCreateInfos[i].queueFamilyIndex = i;
                queueCreateInfos[i].queueCount = 0u;
                queueCreateInfos[i].pQueuePriorities = priorities;
                auto& props = context.properties[i];
                PK_LOG_INFO("Family: %i, NumQueues: %i, Flags: %s", i, props.queueCount, VulkanStr_VkQueueFlags(props.queueFlags));
            }

            PK_LOG_NEWLINE();
        }

        const auto maskTransfer = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
        const auto maskGraphics = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        const auto maskCompute = VK_QUEUE_COMPUTE_BIT;

        typeIndices[(uint32_t)QueueType::Graphics] = GetQueueIndex(context, maskGraphics, maskTransfer, false, true, true);
        typeIndices[(uint32_t)QueueType::Present] = GetQueueIndex(context, maskGraphics, maskTransfer, true, false, false);
        typeIndices[(uint32_t)QueueType::Compute] = GetQueueIndex(context, maskCompute, maskTransfer, false, false, true);
        typeIndices[(uint32_t)QueueType::Transfer] = GetQueueIndex(context, maskTransfer, 0u, false, false, true);
        queueCount = context.selectedCount;

        for (auto i = 0u; i < context.selectedCount; ++i)
        {
            createInfos[i] = queueCreateInfos[context.selectedIndices[i]];
            familyProperties[i] = queueFamilyProperties[context.selectedIndices[i]];
        }

        // This is just for debug naming purposes
        for (auto i = (int32_t)QueueType::MaxCount; i >= 0; --i)
        {
            names[typeIndices[i]] = RHIEnumConvert::QueueTypeToString((QueueType)i);
        }

        {
            PK_LOG_INFO_SCOPE("VulkanQueueSet.Initializer: Selected '%u' Queues From '%u' Physical Device Queue Families:", queueCount, queueFamilyCount);

            for (auto i = 0u; i < queueCount; ++i)
            {
                PK_LOG_INFO("Family: %u", queueFamilies[i]);
            }

            PK_LOG_NEWLINE();
        }
    }


    VulkanQueue::VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, VulkanServiceContext& services, uint32_t queueIndex, const char* name) :
        m_device(device),
        m_family(queueFamily),
        m_queueIndex(queueIndex),
        m_capabilityFlags(VulkanEnumConvert::GetQueueFlagsStageCapabilities(flags)),
        m_barrierHandler(queueFamily),
        m_renderState(services.SetBarrierHandler(&m_barrierHandler))
    {
        vkGetDeviceQueue(m_device, m_family, m_queueIndex, &m_queue);
        VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_QUEUE, (uint64_t)m_queue, FixedString32("PK_Queue_%s", name).c_str());

        VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        commandPoolCreateInfo.queueFamilyIndex = queueFamily;
        VK_ASSERT_RESULT(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPool));
        VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)m_commandPool, FixedString32("PK_Cmd_Pool_%s", name).c_str());

        VkSemaphoreTypeCreateInfo timelineCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = 0ull;

        VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,};
        semaphoreCreateInfo.pNext = &timelineCreateInfo;
        vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_timeline.semaphore);
        VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_timeline.semaphore, FixedString32("PK_Timeline_Semaphore_%s", name).c_str());

        semaphoreCreateInfo.pNext = nullptr;

        for (auto& semaphore : m_semaphores)
        {
            vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &semaphore);
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, FixedString32("PK_Semaphore_%s", name).c_str());
        }

        for (auto& fence : m_commandFences)
        {
            VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            VK_ASSERT_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_FENCE, (uint64_t)fence, FixedString32("PK_Cmd_Fence_%s", name).c_str());
        }
    }

    VulkanQueue::~VulkanQueue()
    {
        WaitCommandBuffers(true);
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroySemaphore(m_device, m_timeline.semaphore, nullptr);

        for (auto& fence : m_commandFences)
        {
            vkDestroyFence(m_device, fence, nullptr);
        }

        for (auto& semaphore : m_semaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }
    }


    FenceRef VulkanQueue::GetFenceRef(int32_t timelineOffset) const
    {
        return FenceRef(this, [](const void* ctx, uint64_t userdata, uint64_t timeout)
            {
                auto queue = reinterpret_cast<const VulkanQueue*>(ctx);
                VkSemaphoreWaitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
                waitInfo.pNext = nullptr;
                waitInfo.flags = VK_SEMAPHORE_WAIT_ANY_BIT;
                waitInfo.semaphoreCount = 1u;
                waitInfo.pSemaphores = &queue->m_timeline.semaphore;
                waitInfo.pValues = &userdata;
                auto result = vkWaitSemaphores(queue->m_device, &waitInfo, timeout);

                if (result != VK_SUCCESS && result != VK_TIMEOUT)
                {
                    VK_THROW_RESULT(result);
                }

                return result == VK_SUCCESS;
            },
            Math::ULongAdd(m_timeline.counter, timelineOffset));
    }

    VulkanCommandBuffer* VulkanQueue::GetCommandBuffer()
    {
        if (m_currentCommandBuffer != nullptr)
        {
            return m_currentCommandBuffer;
        }

        // Allocate all command buffers that have been completed.
        // Doesnt assign references to wrappers (cache warmup essentially).
        // Only primary command buffers are supported.
        VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = m_commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 0u;

        for (auto& commandBuffer : m_commandBuffers)
        {
            allocateInfo.commandBufferCount += (uint32_t)(commandBuffer == VK_NULL_HANDLE);
        }

        if (allocateInfo.commandBufferCount > 0)
        {
            VkCommandBuffer buffers[PK_VK_MAX_COMMAND_BUFFERS];
            VK_ASSERT_RESULT(vkAllocateCommandBuffers(m_device, &allocateInfo, buffers));

            for (uint32_t i = 0u, j = 0u; i < PK_VK_MAX_COMMAND_BUFFERS; ++i)
            {
                if (m_commandBuffers[i] == VK_NULL_HANDLE)
                {
                    m_commandBuffers[i] = buffers[j++];
                }
            }
        }

        for (auto& commandBuffer : m_commandWrappers)
        {
            if (!commandBuffer.IsActive())
            {
                m_currentCommandBuffer = &commandBuffer;
                break;
            }
        }

        PK_DEBUG_THROW_ASSERT(m_currentCommandBuffer, "No available command buffers!");

        auto currentIndex = (int64_t)(m_currentCommandBuffer - m_commandWrappers);
        auto commandBuffer = m_commandBuffers[currentIndex];
        auto fence = m_commandFences[currentIndex];
        m_currentCommandBuffer->BeginRecord(commandBuffer, fence, m_family, &m_renderState);
        return m_currentCommandBuffer;
    }

    VkResult VulkanQueue::Submit(VkSemaphore* outSignal)
    {
        if (m_currentCommandBuffer == nullptr)
        {
            return VK_SUCCESS;
        }

        auto commandBuffer = m_currentCommandBuffer;
        m_currentCommandBuffer->EndRecord();
        m_currentCommandBuffer = nullptr;
        m_timeline.waitFlags = commandBuffer->GetLastCommandStage();

        VkPipelineStageFlags waitFlags[MAX_DEPENDENCIES]{};
        VkSemaphore waits[MAX_DEPENDENCIES]{};
        uint64_t waitValues[MAX_DEPENDENCIES]{};
        uint32_t waitCount = 0u;

        VkSemaphore signals[2]{ m_timeline.semaphore, VK_NULL_HANDLE };
        uint64_t signalValues[2]{ ++m_timeline.counter, 0ull };
        uint32_t signalCount = outSignal ? 2 : 1;

        // Sync swap chain image access if accessed in cmd.
        VkSemaphore imageSignal = commandBuffer->GetImageSignal();

        if (imageSignal != VK_NULL_HANDLE)
        {
            waits[0] = imageSignal;
            waitValues[0] = 0;
            waitFlags[0] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            waitCount++;
        }

        for (auto& timeline : m_waitTimelines)
        {
            if (timeline.semaphore != VK_NULL_HANDLE)
            {
                waits[waitCount] = timeline.semaphore;
                waitValues[waitCount] = timeline.counter;
                waitFlags[waitCount] = timeline.waitFlags & m_capabilityFlags;
                waitCount++;
            }
        }

        if (outSignal)
        {
            *outSignal = GetNextSemaphore();
            signals[1] = *outSignal;
        }

        memset(m_waitTimelines, 0, sizeof(m_waitTimelines));

        VkTimelineSemaphoreSubmitInfo timelineInfo{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
        timelineInfo.waitSemaphoreValueCount = waitCount;
        timelineInfo.pWaitSemaphoreValues = waitValues;
        timelineInfo.signalSemaphoreValueCount = signalCount;
        timelineInfo.pSignalSemaphoreValues = signalValues;

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.pNext = &timelineInfo;
        submitInfo.waitSemaphoreCount = waitCount;
        submitInfo.pWaitSemaphores = waits;
        submitInfo.pWaitDstStageMask = waitFlags;
        submitInfo.signalSemaphoreCount = signalCount;
        submitInfo.pSignalSemaphores = signals;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->GetCommandBuffer();
        return vkQueueSubmit(m_queue, 1, &submitInfo, commandBuffer->GetFence());
    }

    VkResult VulkanQueue::Present(VkSwapchainKHR swapchain, uint32_t imageIndex, uint64_t presentId, VkSemaphore waitSignal)
    {
        auto semaphore = waitSignal ? waitSignal : QueueSignal(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        
        VkPresentIdKHR presentIdKHR{ VK_STRUCTURE_TYPE_PRESENT_ID_KHR };
        presentIdKHR.swapchainCount = 1u;
        presentIdKHR.pPresentIds = &presentId;

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.pNext = &presentIdKHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        return vkQueuePresentKHR(m_queue, &presentInfo);
    }

    VkResult VulkanQueue::BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount)
    {
        m_timeline.waitFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        ++m_timeline.counter;

        VkTimelineSemaphoreSubmitInfo timelineInfo{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
        timelineInfo.waitSemaphoreValueCount = 0;
        timelineInfo.pWaitSemaphoreValues = nullptr;
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues = &m_timeline.counter;

        VkSparseBufferMemoryBindInfo bufferBind{};
        bufferBind.buffer = buffer;
        bufferBind.bindCount = bindCount;
        bufferBind.pBinds = binds;

        VkBindSparseInfo sparseBind{ VK_STRUCTURE_TYPE_BIND_SPARSE_INFO };
        sparseBind.pNext = &timelineInfo;
        sparseBind.bufferBindCount = 1;
        sparseBind.pBufferBinds = &bufferBind;
        sparseBind.pWaitSemaphores = nullptr;
        sparseBind.waitSemaphoreCount = 0;
        sparseBind.pSignalSemaphores = &m_timeline.semaphore;
        sparseBind.signalSemaphoreCount = 1u;
        return vkQueueBindSparse(m_queue, 1, &sparseBind, VK_NULL_HANDLE);
    }

    VkSemaphore VulkanQueue::QueueSignal(VkPipelineStageFlags flags)
    {
        auto semaphore = GetNextSemaphore();

        VkTimelineSemaphoreSubmitInfo timelineInfo{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
        timelineInfo.waitSemaphoreValueCount = 1;
        timelineInfo.pWaitSemaphoreValues = &m_timeline.counter;
        timelineInfo.signalSemaphoreValueCount = 0;
        timelineInfo.pSignalSemaphoreValues = nullptr;

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.pNext = &timelineInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &m_timeline.semaphore;
        submitInfo.pWaitDstStageMask = &m_timeline.waitFlags;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &semaphore;
        submitInfo.commandBufferCount = 0;
        submitInfo.pCommandBuffers = nullptr;

        m_timeline.waitFlags = flags;

        VK_ASSERT_RESULT(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
        return semaphore;
    }

    void VulkanQueue::QueueWait(VkSemaphore semaphore, VkPipelineStageFlags flags)
    {
        for (auto& timeline : m_waitTimelines)
        {
            if (timeline.semaphore == VK_NULL_HANDLE)
            {
                timeline.counter = 0u;
                timeline.semaphore = semaphore;
                timeline.waitFlags = flags;
                break;
            }
        }
    }

    void VulkanQueue::QueueWait(VulkanQueue* other, int32_t timelineOffset)
    {
        if (other != this)
        {
            for (auto& timeline : m_waitTimelines)
            {
                if (timeline.semaphore == VK_NULL_HANDLE)
                {
                    timeline = other->m_timeline;
                    timeline.counter = Math::ULongAdd(timeline.counter, timelineOffset);
                    // Wait at top of pipe as we dont know what the first op will be.
                    timeline.waitFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    break;
                }
            }
        }
    }

    void VulkanQueue::WaitCommandBuffers(bool waitAll)
    {
        VkFence fences[PK_VK_MAX_COMMAND_BUFFERS];
        uint32_t count = 0;

        for (auto& wrapper : m_commandWrappers)
        {
            if (m_currentCommandBuffer != &wrapper)
            {
                if (wrapper.IsActive())
                {
                    fences[count++] = wrapper.GetFence();
                }
                else if (!waitAll)
                {
                    // At least one command buffer has been released/completed.
                    count = 0u;
                    break;
                }
            }
        }

        if (count > 0)
        {
            VK_ASSERT_RESULT(vkWaitForFences(m_device, count, fences, (VkBool32)waitAll, UINT64_MAX));
        }

        for (auto& wrapper : m_commandWrappers)
        {
            if (wrapper.IsActive() && vkGetFenceStatus(m_device, wrapper.GetFence()) == VK_SUCCESS)
            {
                m_commandBuffers[(int64_t)(&wrapper - &m_commandWrappers[0])] = VK_NULL_HANDLE;
                vkFreeCommandBuffers(m_device, m_commandPool, 1, &wrapper.GetCommandBuffer());
                VK_ASSERT_RESULT(vkResetFences(m_device, 1, &wrapper.GetFence()));
                wrapper.FinishExecution();
            }
        }
    }

    void VulkanQueue::Prune()
    {
        m_barrierHandler.Prune();
        WaitCommandBuffers(false);
    }


    VulkanQueueSet::VulkanQueueSet(VkDevice device, const VulkanQueueSetInitializer& initializer, const VulkanServiceContext& services)
    {
        auto servicesCopy = services;

        m_selectedFamilies.count = initializer.queueCount;
        memcpy(m_selectedFamilies.indices, initializer.queueFamilies, sizeof(initializer.queueFamilies));
        memcpy(m_queueIndices, initializer.typeIndices, sizeof(m_queueIndices));

        for (auto i = 0u; i < initializer.queueCount; ++i)
        {
            m_queues[i] = CreateUnique<VulkanQueue>(device, initializer.familyProperties[i].queueFlags, initializer.queueFamilies[i], servicesCopy, 0, initializer.names[i]);
        }
    }

    VkResult VulkanQueueSet::SubmitCurrent(QueueType type, VkSemaphore* outSignal)
    {
        auto queue = GetQueue(type);
        auto result = queue->Submit(outSignal);
        m_lastSubmitFence = queue->GetFenceRef();

        // Sync resource access states to other queues.
        // Not using a single barrier handler for all queues so that queues can be cross recorded.
        for (auto& other : m_queues)
        {
            if (other.get() != queue && other != nullptr)
            {
                auto handlerDst = other->GetBarrierHandler();
                auto handlerSrc = queue->GetBarrierHandler();
                handlerDst->TransferRecords(handlerSrc);
            }
        }

        return result;
    }

    RHICommandBuffer* VulkanQueueSet::Submit(QueueType type)
    {
        VK_ASSERT_RESULT(SubmitCurrent(type));
        return GetCommandBuffer(type);
    }

    void VulkanQueueSet::Wait(QueueType to, QueueType from, int32_t submitOffset)
    {
        GetQueue(to)->QueueWait(GetQueue(from), submitOffset);
    }

    void VulkanQueueSet::Prune()
    {
        for (auto& queue : m_queues)
        {
            if (queue != nullptr)
            {
                queue->Prune();
            }
        }
    }
}
