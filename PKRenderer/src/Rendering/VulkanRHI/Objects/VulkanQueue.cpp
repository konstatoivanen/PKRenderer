#include "PrecompiledHeader.h"
#include "VulkanQueue.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Core/Services/Log.h"
#include <vulkan/vk_enum_string_helper.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::Structs;

    struct QueueFindContext
    {
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHR surface;
        std::vector<VkQueueFamilyProperties>* families;
        std::vector<VkDeviceQueueCreateInfo>* createInfos;
        uint32_t* queueFamilies;
        uint32_t* queueIndices;
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

        for (auto i = 0u; i < ctx.queueCount && !preferSeparate; ++i)
        {
            const auto familyIndex = ctx.queueFamilies[i];
            const auto& family = ctx.families->at(familyIndex);
            auto presentSupport = Utilities::VulkanIsPresentSupported(ctx.physicalDevice, familyIndex, ctx.surface);

            if ((family.queueFlags & requiredFlags) != requiredFlags || 
                (*ctx.createInfos)[familyIndex].queueCount >= family.queueCount ||
                (!presentSupport && requirePresent))
            {
                continue;
            }

            auto value = Math::Functions::CountBits(family.queueFlags & optionalFlags) + (presentSupport && optionalPresent ? 3 : 2);

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
            auto presentSupport = Utilities::VulkanIsPresentSupported(ctx.physicalDevice, i, ctx.surface);

            if ((family.queueFlags & requiredFlags) != requiredFlags ||
                (*ctx.createInfos)[i].queueCount >= family.queueCount ||
                (!presentSupport && requirePresent))
            {
                continue;
            }

            auto value = Math::Functions::CountBits(family.queueFlags & optionalFlags) + (presentSupport && optionalPresent ? 2 : 1);

            if (preferSeparate && (*ctx.createInfos)[i].queueCount == 0)
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

        if (selectedFamily == 0xFFFFFFFF)
        {
            PK_THROW_ERROR("Failed to find queue matching parameters!");
        }

        if (existingIndex != 0xFFFFFFFF)
        {
            return existingIndex;
        }

        PK_THROW_ASSERT(ctx.queueCount < (uint32_t)Structs::QueueType::MaxCount, "Maximum queue count reached!");

        ctx.queueIndices[ctx.queueCount] = (*ctx.createInfos)[selectedFamily].queueCount++;
        ctx.queueFamilies[ctx.queueCount] = selectedFamily;
        return ctx.queueCount++;
    }

    VulkanQueue::VulkanQueue(const VkDevice device, const VkQueueFamilyProperties& properties, uint32_t queueFamily, uint32_t queueIndex) :
        m_device(device),
        m_family(queueFamily),
        m_queueIndex(queueIndex),
        m_familyProperties(properties)
    {
        vkGetDeviceQueue(m_device, m_family, m_queueIndex, &m_queue);
    
        for (auto& semaphore : m_semaphores)
        {
            semaphore = new VulkanSemaphore(m_device);
        }
        
        m_capabilityFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
                            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_capabilityFlags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                                 VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | 
                                 VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | 
                                 VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                                 VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                                 VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | 
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | 
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | 
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | 
                                 VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT | 
                                 VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT |
                                 VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
                                 VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
                                 VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT |
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        }

        if (properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            m_capabilityFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }

        if (properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            m_capabilityFlags |= VK_PIPELINE_STAGE_HOST_BIT |
                                 VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
    }

    VulkanQueue::~VulkanQueue()
    {
        for (auto& semaphore : m_semaphores)
        {
            delete semaphore;
        }
    }

    VkResult VulkanQueue::Present(VkSwapchainKHR swapchain, uint32_t imageIndex)
    {
        SignalInfo signalInfo{};
        QueueDependency(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, &signalInfo, false, true);
        
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = signalInfo.waitCount;
        presentInfo.pWaitSemaphores = signalInfo.waits;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        return vkQueuePresentKHR(m_queue, &presentInfo);
    }

    VkResult VulkanQueue::Submit(Objects::VulkanCommandBuffer* commandBuffer, VkPipelineStageFlags flags, bool waitForPrevious)
    {
        commandBuffer->EndRenderPass();
        VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer->commandBuffer));
        
        SignalInfo signalInfo{};
        QueueDependency(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, &signalInfo, true, waitForPrevious);

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = signalInfo.waits;
        submitInfo.pWaitDstStageMask = signalInfo.waitFlags;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer->commandBuffer;
        submitInfo.signalSemaphoreCount = 1u;
        submitInfo.pSignalSemaphores = &signalInfo.signal;
        return vkQueueSubmit(m_queue, 1, &submitInfo, commandBuffer->fence->vulkanFence);
    }

    VkResult VulkanQueue::BindSparse(VkBuffer buffer, const VkSparseMemoryBind* binds, uint32_t bindCount)
    {
        SignalInfo signalInfo{};
        QueueDependency(VK_PIPELINE_STAGE_NONE, &signalInfo, true, false);

        VkSparseBufferMemoryBindInfo bufferBind{};
        bufferBind.buffer = buffer;
        bufferBind.bindCount = bindCount;
        bufferBind.pBinds = binds;
        
        VkBindSparseInfo sparseBind{ VK_STRUCTURE_TYPE_BIND_SPARSE_INFO };
        sparseBind.bufferBindCount = 1;
        sparseBind.pBufferBinds = &bufferBind;
        sparseBind.pWaitSemaphores = signalInfo.waits;
        sparseBind.waitSemaphoreCount = signalInfo.waitCount;
        sparseBind.pSignalSemaphores = &signalInfo.signal;
        sparseBind.signalSemaphoreCount = 1u;
        return vkQueueBindSparse(m_queue, 1, &sparseBind, VK_NULL_HANDLE);
    }

    void VulkanQueue::QueueDependency(VkPipelineStageFlags flags, SignalInfo* signalInfo, bool addNew, bool waitForPrevious)
    {
        auto semaphore = addNew ? m_semaphores[m_semaphoreIndex++ % MAX_DEPENDENCIES] : nullptr;
        
        if (signalInfo)
        {
            signalInfo->waitCount = 0u;

            if (waitForPrevious)
            {
                m_signalGroups[1] = m_signalGroups[0];
                m_signalGroups[0] = {};
                signalInfo->waitCount = m_signalGroups[1].count;
            }
            
            signalInfo->waits = m_signalGroups[1].semaphores;
            signalInfo->waitFlags = m_signalGroups[1].flags;
            signalInfo->signal = semaphore->vulkanSemaphore;
        }

        if (addNew)
        {
            auto& group = m_signalGroups[0];
            group.semaphores[group.count] = semaphore->vulkanSemaphore;
            group.flags[group.count++] = flags;
        }
    }

    VulkanQueueSet::VulkanQueueSet(VkDevice device, const Initializer& initializer)
    {
        for (auto i = 0u; i < initializer.queueCount; ++i)
        {
            auto& familyProps = initializer.familyProperties.at(initializer.queueFamilies[i]);
            m_queues[i] = new VulkanQueue(device, familyProps, initializer.queueFamilies[i], initializer.queueIndices[i]);
        }

        memcpy(m_queueIndices, initializer.typeIndices, sizeof(m_queueIndices));
    }

    VulkanQueueSet::~VulkanQueueSet()
    {
        for (auto i = 0u; i < (uint32_t)Structs::QueueType::MaxCount; ++i)
        {
            if (m_queues[i])
            {
                delete m_queues[i];
            }
        }
    }

    VulkanQueueSet::Initializer::Initializer(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        createInfos.clear();

        familyProperties = Utilities::VulkanGetPhysicalDeviceQueueFamilyProperties(physicalDevice);

        QueueFindContext context;
        context.families = &familyProperties;
        context.physicalDevice = physicalDevice;
        context.surface = surface;
        context.queueFamilies = queueFamilies;
        context.queueIndices = queueIndices;
        context.createInfos = &createInfos;

        PK_LOG_VERBOSE("Found %i queues:", context.families->size());

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
            PK_LOG_VERBOSE("    NumQueues: %i, Flags: %s", props.queueCount, flagsString.c_str());
        }

        auto maskTransfer = VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
        auto maskGraphics = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        auto maskCompute = VK_QUEUE_COMPUTE_BIT;

        typeIndices[(uint32_t)QueueType::Graphics] = GetQueueIndex(context, maskGraphics, maskTransfer, false, true, true);
        typeIndices[(uint32_t)QueueType::Present] = GetQueueIndex(context, maskGraphics, maskTransfer, true, false, false);
        typeIndices[(uint32_t)QueueType::Transfer] = GetQueueIndex(context, maskTransfer, 0u, false, false, false);
        typeIndices[(uint32_t)QueueType::ComputeAsync] = GetQueueIndex(context, maskCompute, maskTransfer, false, false, true);
        queueCount = context.queueCount;

        for (int32_t i = (int32_t)createInfos.size() - 1; i >= 0; --i)
        {
            if (createInfos.at(i).queueCount == 0)
            {
                createInfos.erase(createInfos.begin() + i);
            }
        }
    }
}