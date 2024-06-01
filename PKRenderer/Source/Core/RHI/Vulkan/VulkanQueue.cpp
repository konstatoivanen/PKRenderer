#include "PrecompiledHeader.h"
#include <vulkan/vk_enum_string_helper.h>
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
        std::vector<VkQueueFamilyProperties>* families;
        std::vector<VkDeviceQueueCreateInfo>* createInfos;
        uint32_t* queueFamilies;
        uint32_t queueCount = 0u;
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

        for (auto i = 0u; i < ctx.queueCount; ++i)
        {
            const auto familyIndex = ctx.queueFamilies[i];
            const auto& family = ctx.families->at(familyIndex);
            auto presentSupport = VulkanIsPresentSupported(ctx.physicalDevice, familyIndex, ctx.surface);

            if ((family.queueFlags & requiredFlags) != requiredFlags ||
                (!presentSupport && requirePresent))
            {
                continue;
            }

            auto value = Math::CountBits(family.queueFlags & optionalFlags) + (presentSupport && optionalPresent ? 2 : 1);

            if (value > heuristic)
            {
                heuristic = value;
                selectedFamily = familyIndex;
                existingIndex = i;
            }
        }

        for (auto i = 0u; i < ctx.families->size(); ++i)
        {
            auto& family = ctx.families->at(i);
            auto presentSupport = VulkanIsPresentSupported(ctx.physicalDevice, i, ctx.surface);

            if ((family.queueFlags & requiredFlags) != requiredFlags ||
                (*ctx.createInfos)[i].queueCount > 0 ||
                (!presentSupport && requirePresent))
            {
                continue;
            }

            auto value = Math::CountBits(family.queueFlags & optionalFlags) + (presentSupport && optionalPresent ? 2 : 1);

            if (preferSeparate)
            {
                value++;
            }

            if (value > heuristic)
            {
                heuristic = value;
                selectedFamily = i;
                existingIndex = 0xFFFFFFFF;
            }
        }

        PK_THROW_ASSERT(selectedFamily != 0xFFFFFFFF, "Failed to find queue matching parameters!");

        if (existingIndex != 0xFFFFFFFF)
        {
            return existingIndex;
        }

        (*ctx.createInfos)[selectedFamily].queueCount = 1;
        ctx.queueFamilies[ctx.queueCount] = selectedFamily;
        return ctx.queueCount++;
    }


    VulkanQueue::VulkanQueue(const VkDevice device, VkQueueFlags flags, uint32_t queueFamily, const VulkanServiceContext& services, uint32_t queueIndex) :
        m_device(device),
        m_family(queueFamily),
        m_queueIndex(queueIndex)
    {
        vkGetDeviceQueue(m_device, m_family, m_queueIndex, &m_queue);

        VkSemaphoreTypeCreateInfo timelineCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = 0ull;

        VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,};
        createInfo.pNext = &timelineCreateInfo;

        vkCreateSemaphore(m_device, &createInfo, nullptr, &m_timeline.semaphore);

        createInfo.pNext = nullptr;

        for (auto& semaphore : m_semaphores)
        {
            vkCreateSemaphore(m_device, &createInfo, nullptr, &semaphore);
        }

        m_capabilityFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_capabilityFlags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                // Requires tesselation & geometry features
                //VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                //VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                //VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | 
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
                //VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT | 
                //VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT |
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;// |
                //VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT |
                //VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        }

        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            m_capabilityFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }

        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            m_capabilityFlags |= //VK_PIPELINE_STAGE_HOST_BIT |
                VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        auto servicesCopy = services;
        barrierHandler = CreateScope<VulkanBarrierHandler>(m_family);
        servicesCopy.barrierHandler = barrierHandler.get();
        commandPool = CreateScope<VulkanCommandBufferPool>(device, servicesCopy, queueFamily, m_capabilityFlags);
    }

    VulkanQueue::~VulkanQueue()
    {
        vkDestroySemaphore(m_device, m_timeline.semaphore, nullptr);

        for (auto& semaphore : m_semaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }
    }

    VkResult VulkanQueue::Present(VkSwapchainKHR swapchain, uint32_t imageIndex, VkSemaphore waitSignal)
    {
        auto semaphore = waitSignal ? waitSignal : QueueSignal(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        return vkQueuePresentKHR(m_queue, &presentInfo);
    }

    VkResult VulkanQueue::Submit(VulkanCommandBuffer* commandBuffer, VkSemaphore* outSignal)
    {
        if (!commandBuffer)
        {
            return VK_SUCCESS;
        }

        VkPipelineStageFlags waitFlags[MAX_DEPENDENCIES]{};
        VkSemaphore waits[MAX_DEPENDENCIES]{};
        uint64_t waitValues[MAX_DEPENDENCIES]{};
        uint32_t waitCount = 0u;

        VkSemaphore signals[2]{ m_timeline.semaphore, VK_NULL_HANDLE };
        uint64_t signalValues[2]{ ++m_timeline.counter, 0ull };
        uint32_t signalCount = outSignal ? 2 : 1;

        for (auto& timeline : m_waitTimelines)
        {
            if (timeline.semaphore == VK_NULL_HANDLE)
            {
                break;
            }

            waits[waitCount] = timeline.semaphore;
            waitValues[waitCount] = timeline.counter;
            waitFlags[waitCount] = timeline.waitFlags & m_capabilityFlags;
            waitCount++;
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
        submitInfo.pCommandBuffers = &commandBuffer->GetNative();

        if (outSignal)
        {
            *outSignal = GetNextSemaphore();
            signals[1] = *outSignal;
        }

        m_timeline.waitFlags = commandBuffer->GetLastCommandStage();
        return vkQueueSubmit(m_queue, 1, &submitInfo, commandBuffer->GetFence());
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
            if (timeline.semaphore != VK_NULL_HANDLE)
            {
                continue;
            }

            timeline.counter = 0u;
            timeline.semaphore = semaphore;
            timeline.waitFlags = flags;
            break;
        }
    }

    void VulkanQueue::QueueWait(VulkanQueue* other, int32_t timelineOffset)
    {
        for (auto& timeline : m_waitTimelines)
        {
            if (timeline.semaphore != VK_NULL_HANDLE)
            {
                continue;
            }

            timeline = other->m_timeline;
            timeline.counter = Math::ULongAdd(timeline.counter, timelineOffset);
            // Wait at top of pipe as we dont know what the first op will be.
            timeline.waitFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
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


    VulkanQueueSet::VulkanQueueSet(VkDevice device, const Initializer& initializer, const VulkanServiceContext& services)
    {
        auto servicesCopy = services;
        m_selectedFamilies.count = initializer.queueCount;

        for (auto i = 0u; i < initializer.queueCount; ++i)
        {
            auto& familyProps = initializer.familyProperties.at(initializer.queueFamilies[i]);
            m_queues[i] = CreateScope<VulkanQueue>(device, familyProps.queueFlags, initializer.queueFamilies[i], servicesCopy);
            m_selectedFamilies.indices[i] = initializer.queueFamilies[i];
        }

        memcpy(m_queueIndices, initializer.typeIndices, sizeof(m_queueIndices));
    }

    void VulkanQueueSet::Prune()
    {
        for (auto& queue : m_queues)
        {
            if (queue != nullptr)
            {
                queue->barrierHandler->Prune();
                queue->commandPool->Prune(false);
            }
        }
    }

    VkResult VulkanQueueSet::SubmitCurrent(QueueType type, VkSemaphore* outSignal)
    {
        auto queue = GetQueue(type);
        return queue->Submit(GetQueue(type)->commandPool->EndCurrent(), outSignal);
    }

    RHICommandBuffer* VulkanQueueSet::Submit(QueueType type)
    {
        VK_ASSERT_RESULT(SubmitCurrent(type));
        return GetCommandBuffer(type);
    }

    void VulkanQueueSet::Sync(QueueType from, QueueType to, int32_t submitOffset)
    {
        auto queueFrom = GetQueue(from);
        auto queueTo = GetQueue(to);
        queueTo->QueueWait(queueFrom, submitOffset);
        queueFrom->barrierHandler->TransferRecords(queueTo->barrierHandler.get());
    }

    void VulkanQueueSet::Wait(QueueType from, QueueType to, int32_t submitOffset)
    {
        auto queueFrom = GetQueue(from);
        auto queueTo = GetQueue(to);
        queueTo->QueueWait(queueFrom, submitOffset);
    }

    void VulkanQueueSet::Transfer(QueueType from, QueueType to)
    {
        auto queueFrom = GetQueue(from);
        auto queueTo = GetQueue(to);
        queueFrom->barrierHandler->TransferRecords(queueTo->barrierHandler.get());
    }

    VulkanQueueSet::Initializer::Initializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        createInfos.clear();

        familyProperties = VulkanGetPhysicalDeviceQueueFamilyProperties(physicalDevice);

        QueueFindContext context;
        context.families = &familyProperties;
        context.physicalDevice = physicalDevice;
        context.surface = surface;
        context.queueFamilies = queueFamilies;
        context.createInfos = &createInfos;

        PK_LOG_INFO("VulkanQueueSet.Initializer: Found '%i' Physical Device Queue Families:", context.families->size());
        PK_LOG_ADD_INDENT();

        for (auto i = 0u; i < context.families->size(); ++i)
        {
            VkDeviceQueueCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            createInfo.flags = 0u;
            createInfo.queueFamilyIndex = i;
            createInfo.queueCount = 0u;
            createInfo.pQueuePriorities = priorities;
            createInfos.push_back(createInfo);

            auto& props = context.families->at(i);
            auto flagsString = string_VkQueueFlags(props.queueFlags);
            PK_LOG_INFO("Family: %i, NumQueues: %i, Flags: %s", i, props.queueCount, flagsString.c_str());
        }

        PK_LOG_SUB_INDENT();
        PK_LOG_NEWLINE();

        auto maskTransfer = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
        auto maskGraphics = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        auto maskCompute = VK_QUEUE_COMPUTE_BIT;

        typeIndices[(uint32_t)QueueType::Graphics] = GetQueueIndex(context, maskGraphics, maskTransfer, false, true, true);
        typeIndices[(uint32_t)QueueType::Present] = GetQueueIndex(context, maskGraphics, maskTransfer, true, false, false);
        typeIndices[(uint32_t)QueueType::Compute] = GetQueueIndex(context, maskCompute, maskTransfer, false, false, true);
        typeIndices[(uint32_t)QueueType::Transfer] = GetQueueIndex(context, maskTransfer, 0u, false, false, true);
        queueCount = context.queueCount;

        for (auto i = (int32_t)createInfos.size() - 1; i >= 0; --i)
        {
            if (createInfos.at(i).queueCount == 0)
            {
                createInfos.erase(createInfos.begin() + i);
            }
        }

        PK_LOG_INFO("VulkanQueueSet.Initializer: Selected '%i' Queues From '%i' Physical Device Queue Families:", queueCount, createInfos.size());
        PK_LOG_ADD_INDENT();

        for (auto i = 0u; i < queueCount; ++i)
        {
            PK_LOG_INFO("Family: %i", queueFamilies[i]);
        }

        PK_LOG_SUB_INDENT();
        PK_LOG_NEWLINE();
    }
}
