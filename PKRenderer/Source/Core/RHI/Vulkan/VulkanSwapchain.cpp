#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanSwapchain.h"

namespace PK
{
    VulkanSwapchain::VulkanSwapchain(VulkanDriver* driver, const SwapchainDescriptor& descriptor) : m_driver(driver)
    {
        VK_ASSERT_RESULT_CTX(VulkanCreateSurfaceKHR(driver->instance, descriptor.nativeWindowHandle, &m_surface), "Failed to create window surface!");

        auto presentFamily = m_driver->queues->GetQueue(QueueType::Present)->GetFamily();
        PK_FATAL_ASSERT(VulkanIsPresentSupported(driver->physicalDevice, presentFamily, m_surface), "Surface present not supported on selected physical device!");
        
        Rebuild(descriptor);
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        WaitForPresent(0u, UINT64_MAX);
        Release();
        vkDestroySurfaceKHR(m_driver->instance, m_surface, nullptr);
    }

    void VulkanSwapchain::SetDesiredResolution(const uint2& resolution)
    {
        if (resolution.x != 0u && resolution.y != 0u && (resolution.x != m_extent.width || resolution.y != m_extent.height))
        {
            m_descriptor.desiredResolution = resolution;
            m_outofdate = true;
        }
    }

    void VulkanSwapchain::SetDesiredFormat(TextureFormat format)
    {
        if (m_descriptor.desiredFormat != format)
        {
            m_descriptor.desiredFormat = format;
            m_outofdate = true;
        }
    }

    void VulkanSwapchain::SetDesiredColorSpace(ColorSpace colorSpace)
    {
        if (m_descriptor.desiredColorSpace != colorSpace)
        {
            m_descriptor.desiredColorSpace = colorSpace;
            m_outofdate = true;
        }
    }

    void VulkanSwapchain::SetDesiredVSyncMode(VSyncMode vsyncMode)
    {
        if (m_descriptor.desiredVSyncMode != vsyncMode)
        {
            m_descriptor.desiredVSyncMode = vsyncMode;

            const auto desiredPresentMode = VulkanEnumConvert::GetPresentMode(vsyncMode);

            for (auto i = 0u; i < m_presentModeCount; ++i)
            {
                if (m_presentModes[i] == desiredPresentMode)
                {
                    m_presentMode = desiredPresentMode;
                }
            }

            if (m_presentMode != desiredPresentMode)
            {
                m_outofdate = true;
            }
        }
    }

    bool VulkanSwapchain::AcquireFullScreen(const void* nativeMonitor)
    {
        // Has exclusive full screen for the monitor
        if (m_descriptor.nativeMonitorHandle == nativeMonitor)
        {
            return m_descriptor.nativeMonitorHandle != nullptr;
        }

        // Releasing full screen defer update to next acquire image.
        if (m_descriptor.nativeMonitorHandle && !nativeMonitor)
        {
            m_outofdate = true;
            return false;
        }

        // Try to rebuild and acquire full screen.
        if (!m_descriptor.nativeMonitorHandle && nativeMonitor)
        {
            auto descriptor = m_descriptor;
            descriptor.nativeMonitorHandle = nativeMonitor;
            m_outofdate = true;
            Rebuild(descriptor);
        }

        // Return current state
        return m_descriptor.nativeMonitorHandle != nullptr;
    }


    void VulkanSwapchain::WaitForPresent(uint32_t historyOffset, uint64_t timeoutNanos)
    {
        if (m_presentId > historyOffset && m_swapchain != VK_NULL_HANDLE)
        {
            vkWaitForPresentKHR(m_driver->device, m_swapchain, m_presentId - historyOffset, timeoutNanos);
        }
    }

    void VulkanSwapchain::Release()
    {
        if (m_descriptor.nativeMonitorHandle != nullptr)
        {
            VK_ASSERT_RESULT(vkReleaseFullScreenExclusiveModeEXT(m_driver->device, m_swapchain));
            m_descriptor.nativeMonitorHandle = nullptr;
        }

        for (auto i = 0u; i < PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT; ++i)
        {
            if (m_imageViews[i] != nullptr)
            {
                m_driver->DeletePooled(m_imageViews[i]);
                m_imageViews[i] = nullptr;

                vkDestroySemaphore(m_driver->device, m_semaphoresPresent[i], nullptr);
                vkDestroySemaphore(m_driver->device, m_semaphoresAcquire[i], nullptr);
            }
        }

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_driver->device, m_swapchain, nullptr);
        }
    }

    void VulkanSwapchain::Rebuild(const SwapchainDescriptor& descriptor)
    {
        PK_LOG_INFO_FUNC();

        // Wait for last present so that we can safely release this.
        WaitForPresent(0u, UINT64_MAX);
        RHI::WaitForIdle();
        Release();

        const auto desiredFormat = VulkanEnumConvert::GetFormat(descriptor.desiredFormat);
        const auto desiredColorSpace = VulkanEnumConvert::GetColorSpace(descriptor.desiredColorSpace);
        const VkExtent2D desiredExtent = { descriptor.desiredResolution.x, descriptor.desiredResolution.y };

        auto queueGraphics = m_driver->queues->GetQueue(QueueType::Graphics);
        auto queuePresent = m_driver->queues->GetQueue(QueueType::Present);

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_driver->physicalDevice, m_surface, &capabilities);
        m_format = VulkanSelectSurfaceFormat(m_driver->physicalDevice, m_surface, desiredFormat, desiredColorSpace);
        m_extent = VulkanSelectSurfaceExtent(capabilities, desiredExtent);

        m_presentMode = VulkanEnumConvert::GetPresentMode(descriptor.desiredVSyncMode);
        m_presentModeCount = VulkanQueryPresentModes(m_driver->physicalDevice, m_surface, &m_presentMode, m_presentModes, (uint32_t)VSyncMode::EnumCount);

        auto maxImageCount = capabilities.maxImageCount > 0 ? capabilities.maxImageCount : UINT32_MAX;
        auto minImageCount = capabilities.minImageCount;
        maxImageCount = math::min(PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT, maxImageCount);

        m_imageCount = math::clamp(descriptor.desiredImageCount, minImageCount, maxImageCount);
        uint32_t queueFamilyIndices[] = { queueGraphics->GetFamily(), queuePresent->GetFamily() };

        auto fullscreenInfo = VulkanGetSwapchainFullscreenInfo(descriptor.nativeMonitorHandle, descriptor.nativeMonitorHandle != nullptr);

        VkSwapchainPresentScalingCreateInfoEXT presentScalingInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT };
        presentScalingInfo.pNext = fullscreenInfo.swapchainPNext;
        presentScalingInfo.scalingBehavior = VK_PRESENT_SCALING_STRETCH_BIT_EXT;
        presentScalingInfo.presentGravityX = VK_PRESENT_GRAVITY_CENTERED_BIT_EXT;
        presentScalingInfo.presentGravityY = VK_PRESENT_GRAVITY_CENTERED_BIT_EXT;

        VkSwapchainPresentModesCreateInfoKHR presentModes{ VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_KHR };
        presentModes.pNext = &presentScalingInfo;
        presentModes.presentModeCount = m_presentModeCount;
        presentModes.pPresentModes = m_presentModes;

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.pNext = &presentModes;
        swapchainCreateInfo.flags = VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_EXT;
        swapchainCreateInfo.surface = m_surface;
        swapchainCreateInfo.minImageCount = m_imageCount;
        swapchainCreateInfo.imageFormat = m_format.format;
        swapchainCreateInfo.imageColorSpace = m_format.colorSpace;
        swapchainCreateInfo.imageExtent = m_extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        // Not using old swapchain anymore due to dwm yielding permanent invalid state.
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;

        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        swapchainCreateInfo.preTransform = capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = m_presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_ASSERT_RESULT_CTX(vkCreateSwapchainKHR(m_driver->device, &swapchainCreateInfo, nullptr, &m_swapchain), "failed to create swap chain!");

        PK_LOG_INFO("Width: %i", m_extent.width);
        PK_LOG_INFO("Height: %i", m_extent.height);
        PK_LOG_INFO("Images: %i", m_imageCount);
        PK_LOG_INFO("Format: %s", VulkanCStr_VkFormat(m_format.format));
        PK_LOG_INFO("Color Space: %s", VulkanCStr_VkColorSpaceKHR(m_format.colorSpace));
        PK_LOG_INFO("Present Mode: %s", VulkanCStr_VkPresentModeKHR(m_presentMode));

        vkGetSwapchainImagesKHR(m_driver->device, m_swapchain, &m_imageCount, nullptr);
        vkGetSwapchainImagesKHR(m_driver->device, m_swapchain, &m_imageCount, m_images);

        for (auto i = 0u; i < m_imageCount; ++i)
        {
            VulkanImageViewCreateInfo info;
            info.image = m_images[i];
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            info.format = m_format.format;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.components = VkComponentMapping{};
            info.extent = { m_extent.width, m_extent.height, 1 };
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;
            FixedString64 name("Swapchain.Image%u", i);
            m_imageViews[i] = m_driver->CreatePooled<VulkanImageView>(m_driver->device, info, name.c_str());

            VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, };
            vkCreateSemaphore(m_driver->device, &semaphoreCreateInfo, nullptr, &m_semaphoresPresent[i]);
            VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_semaphoresPresent[i], "PK_Swapchain_Present_Semaphore");
            vkCreateSemaphore(m_driver->device, &semaphoreCreateInfo, nullptr, &m_semaphoresAcquire[i]);
            VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_semaphoresAcquire[i], "PK_Swapchain_Acquire_Semaphore");
        }

        m_descriptor = descriptor;

        // Failed to acquire full screen set monitor to null.
        if (descriptor.nativeMonitorHandle && vkAcquireFullScreenExclusiveModeEXT(m_driver->device, m_swapchain) != VK_SUCCESS)
        {
            m_descriptor.nativeMonitorHandle = nullptr;
        }

        m_outofdate = false;
        m_presentId = 0ull;
        m_acquireSignal = VK_NULL_HANDLE;
        m_presentSignal = VK_NULL_HANDLE;
    }
    

    VkSemaphore VulkanSwapchain::ConsumeAcquireSignal()
    {
        auto signal = m_acquireSignal;
        m_acquireSignal = VK_NULL_HANDLE;
        return signal;
    }

    VkSemaphore VulkanSwapchain::ConsumePresentSignal()
    {
        auto signal = m_presentSignal;
        m_presentSignal = VK_NULL_HANDLE; 
        return signal;
    }

    void VulkanSwapchain::Validate()
    {
        if (m_outofdate)
        {
            Rebuild(m_descriptor);
        }
    }

    void VulkanSwapchain::AcquireNextImage()
    {
        if (m_acquireSignal == VK_NULL_HANDLE)
        {
            // Partially works in fixing the bellow issue. but causes issues with 60hz monitors.
            WaitForPresent(m_imageCount - 1u, UINT64_MAX);

            // @TODO NV by default uses DXGI layered swapchain. the present of which is not visible to the application
            // In this mode the imaqe acquire signal only waits for the vk submit usage and not the full present.
            // This causes the frame time to accumulate unevenly and an eventual long wait when the dxgi swapchain has to actually wait for images.
            m_acquireSignal = m_semaphoresAcquire[m_presentId % m_imageCount];
            VK_ASSERT_RESULT(vkAcquireNextImageKHR(m_driver->device, m_swapchain, UINT64_MAX, m_acquireSignal, VK_NULL_HANDLE, &m_imageIndex));
            m_presentSignal = m_semaphoresPresent[m_imageIndex];
        }
    }

    void VulkanSwapchain::Present(VulkanQueue* queue, VkSemaphore presentSignal)
    {
        m_acquireSignal = VK_NULL_HANDLE;
        m_presentSignal = VK_NULL_HANDLE;
        m_hasExternalFrameFence = false;
        m_frameIndex = (m_frameIndex + 1) % PK_RHI_MAX_FRAMES_IN_FLIGHT;
        m_presentId++;
        queue->Present(m_swapchain, m_imageIndex, m_presentId, m_presentMode, presentSignal);
    }
}
