#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanSwapchain.h"

namespace PK
{
    VulkanSwapchain::VulkanSwapchain(VulkanDriver* driver, const SwapchainDescriptor& descriptor) : m_driver(driver)
    {
        VK_ASSERT_RESULT_CTX(VulkanCreateSurfaceKHR(driver->instance, descriptor.nativeWindowHandle, &m_surface), "Failed to create window surface!");

        auto presentFamily = m_driver->queues->GetQueue(QueueType::Present)->GetFamily();

        VkBool32 presentSupported;
        VK_ASSERT_RESULT_CTX(vkGetPhysicalDeviceSurfaceSupportKHR(driver->physicalDevice, presentFamily, m_surface, &presentSupported), "Surface support query failure!");
        PK_THROW_ASSERT(presentSupported, "Surface present not supported on selected physical device!");

        Rebuild(descriptor);
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        m_driver->WaitForIdle();
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
            m_outofdate = true;
        }
    }

    void VulkanSwapchain::SetFrameFence(const FenceRef& fence)
    {
        m_frameFences[m_frameIndex] = fence;
        m_hasExternalFrameFence = true;
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

    bool VulkanSwapchain::AcquireNextImage()
    {
        if (m_outofdate)
        {
            Rebuild(m_descriptor);
        }

        // This is not strictly necesssary when not using double buffering for staging buffers.
        // However, for them to have coherent memory operations we cannot push more than 2 frames at a time.
        PK_THROW_ASSERT(m_frameFences[m_frameIndex].WaitInvalidate(UINT64_MAX), "Frame fence timeout!");

        auto queueGraphics = m_driver->queues->GetQueue(QueueType::Graphics);

        // Dont spam sepmaphore acquires if we're stuck in an invalidation loop.
        if (m_imageAvailableSignal == VK_NULL_HANDLE)
        {
            m_imageAvailableSignal = queueGraphics->GetNextSemaphore();
        }

        auto result = vkAcquireNextImageKHR(m_driver->device, m_swapchain, UINT64_MAX, m_imageAvailableSignal, VK_NULL_HANDLE, &m_imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR && !m_outofdate)
        {
            m_outofdate = true;
            PK_LOG_RHI("Swap chain image acquire failed! Swap chain is out of date!");
        }

        if (result == VK_SUBOPTIMAL_KHR && !m_suboptimal)
        {
            m_suboptimal = true;
            PK_LOG_RHI("Swap chain is sub optimal!");
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
        {
            VK_THROW_RESULT_CTX(result, "Failed to acquire swap chain image: ");
        }

        return !m_outofdate;
    }

    void VulkanSwapchain::Present()
    {
        auto queueGraphics = m_driver->queues->GetQueue(QueueType::Graphics);
        auto queuePresent= m_driver->queues->GetQueue(QueueType::Present);

        // Swapchain write is expected to be in the last (and implicit) graphics submit.
        // @TODO implicit submit sounds bad?!?
        VkSemaphore waitSignal = VK_NULL_HANDLE;
        queueGraphics->QueueWait(m_imageAvailableSignal, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        queueGraphics->commandPool->GetCurrent()->ValidateSwapchainPresent(this);
        m_driver->queues->SubmitCurrent(QueueType::Graphics, &waitSignal);

        // Frame synchronization for this frame is handled externally.
        if (!m_hasExternalFrameFence)
        {
            // Assumes previous submit was for rendering work.
            m_frameFences[m_frameIndex] = queueGraphics->GetFenceRef();
        }

        m_imageAvailableSignal = VK_NULL_HANDLE;
        m_hasExternalFrameFence = false;
        m_frameIndex = (m_frameIndex + 1) % PK_RHI_MAX_FRAMES_IN_FLIGHT;
        VK_ASSERT_RESULT(queuePresent->Present(m_swapchain, m_imageIndex, waitSignal));
    }

    void VulkanSwapchain::Release()
    {
        if (m_descriptor.nativeMonitorHandle != nullptr)
        {
            VK_ASSERT_RESULT(vkReleaseFullScreenExclusiveModeEXT(m_driver->device, m_swapchain));
            m_descriptor.nativeMonitorHandle = nullptr;
        }

        for (size_t i = 0u; i < MaxImageCount; ++i)
        {
            if (m_imageViews[i] != nullptr)
            {
                RHIDriver::Get()->GetNative<VulkanDriver>()->imageViewPool.Delete(m_imageViews[i]);
                m_imageViews[i] = nullptr;
            }
        }

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_driver->device, m_swapchain, nullptr);
        }
    }

    void VulkanSwapchain::Rebuild(const SwapchainDescriptor& descriptor)
    {
        PK_LOG_INFO_FUNC("");

        vkDeviceWaitIdle(m_driver->device);
        Release();

        const auto desiredFormat = VulkanEnumConvert::GetFormat(descriptor.desiredFormat);
        const auto desiredColorSpace = VulkanEnumConvert::GetColorSpace(descriptor.desiredColorSpace);
        const auto desiredPresentMode = VulkanEnumConvert::GetPresentMode(descriptor.desiredVSyncMode);
        const VkExtent2D desiredExtent = { descriptor.desiredResolution.x, descriptor.desiredResolution.y };

        auto queueGraphics = m_driver->queues->GetQueue(QueueType::Graphics);
        auto queuePresent = m_driver->queues->GetQueue(QueueType::Present);

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_driver->physicalDevice, m_surface, &capabilities);
        auto availableFormats = VulkanGetPhysicalDeviceSurfaceFormatsKHR(m_driver->physicalDevice, m_surface);
        auto availablePresentModes = VulkanGetPhysicalDeviceSurfacePresentModesKHR(m_driver->physicalDevice, m_surface);
        m_format = VulkanSelectSurfaceFormat(availableFormats, desiredFormat, desiredColorSpace);
        m_presentMode = VulkanSelectPresentMode(availablePresentModes, desiredPresentMode);
        m_extent = VulkanSelectSurfaceExtent(capabilities, desiredExtent);

        auto maxImageCount = capabilities.maxImageCount > 0 ? capabilities.maxImageCount : UINT32_MAX;
        auto minImageCount = capabilities.minImageCount;
        maxImageCount = maxImageCount > MaxImageCount ? MaxImageCount : maxImageCount;

        m_imageCount = glm::clamp(PK_RHI_DESIRED_SWAP_CHAIN_IMAGE_COUNT, minImageCount, maxImageCount);
        uint32_t queueFamilyIndices[] = { queueGraphics->GetFamily(), queuePresent->GetFamily() };

        auto fullscreenInfo = VulkanGetSwapchainFullscreenInfo(descriptor.nativeMonitorHandle, descriptor.nativeMonitorHandle != nullptr);

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.pNext = fullscreenInfo.swapchainPNext;
        swapchainCreateInfo.surface = m_surface;
        swapchainCreateInfo.minImageCount = m_imageCount;
        swapchainCreateInfo.imageFormat = m_format.format;
        swapchainCreateInfo.imageColorSpace = m_format.colorSpace;
        swapchainCreateInfo.imageExtent = m_extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
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

        for (size_t i = 0u; i < m_imageCount; ++i)
        {
            VulkanImageViewCreateInfo info;
            info.image = m_images[i];
            info.imageAlias = VK_NULL_HANDLE;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = m_format.format;
            info.formatAlias = m_format.format;
            info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.components = VkComponentMapping{};
            info.extent = { m_extent.width, m_extent.height, 1 };
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;
            info.isConcurrent = false;
            info.isTracked = true;
            info.isAlias = false;

            FixedString64 name("Swapchain.Image%lli", i);
            m_imageViews[i] = m_driver->imageViewPool.New(m_driver->device, info, name.c_str());
        }

        for (auto& fence : m_frameFences)
        {
            fence.Invalidate();
        }

        m_descriptor = descriptor;

        // Failed to acquire full screen set monitor to null.
        if (descriptor.nativeMonitorHandle && vkAcquireFullScreenExclusiveModeEXT(m_driver->device, m_swapchain) != VK_SUCCESS)
        {
            m_descriptor.nativeMonitorHandle = nullptr;
        }

        m_outofdate = false;
    }

}