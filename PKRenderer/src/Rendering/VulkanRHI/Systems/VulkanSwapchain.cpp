#include "PrecompiledHeader.h"
#include "VulkanSwapchain.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Math/PKMath.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI::Systems
{
    static bool SwapchainErrorAssert(VkResult result, bool& outofdate, bool& submoptimal)
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR && !outofdate)
        {
            outofdate = true;
            PK_LOG_VERBOSE("Swap chain image acquire failed! Swap chain is out of date!");
        }

        if (result == VK_SUBOPTIMAL_KHR && !submoptimal)
        {
            submoptimal = true;
            PK_LOG_VERBOSE("Swap chain is sub optimal!");
        }

        return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
    }

    VulkanSwapchain::VulkanSwapchain(const VkPhysicalDevice physicalDevice, 
                                     const VkDevice device, 
                                     const VkSurfaceKHR surface,
                                     uint32_t queueIndexGraphics, 
                                     uint32_t queueIndexPresent,
                                     const SwapchainCreateInfo& createInfo) :
                                        m_physicalDevice(physicalDevice),
                                        m_device(device),
                                        m_surface(surface),
                                        m_queueIndexGraphics(queueIndexGraphics),
                                        m_queueIndexPresent(queueIndexPresent)

    {
        vkGetDeviceQueue(device, queueIndexPresent, 0, &m_presentQueue);
        Rebuild(createInfo);
    }

    void VulkanSwapchain::Rebuild(const SwapchainCreateInfo& createInfo)
    {
        m_cachedCreateInfo = createInfo;

        vkDeviceWaitIdle(m_device);

        Release();

        m_maxFramesInFlight = createInfo.maxFramesInFlight;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
        auto availableFormats = Utilities::VulkanGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface);
        auto availablePresentModes = Utilities::VulkanGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface);
        auto surfaceFormat = Utilities::VulkanSelectSurfaceFormat(availableFormats, createInfo.desiredFormat, createInfo.desiredColorSpace);
        auto presentMode = Utilities::VulkanSelectPresentMode(availablePresentModes, createInfo.desiredPresentMode);
        auto extent = Utilities::VulkanSelectSurfaceExtent(capabilities, createInfo.desiredExtent);

        uint32_t maxImageCount = capabilities.maxImageCount > 0 ? capabilities.maxImageCount : UINT32_MAX;
        uint32_t minImageCount = capabilities.minImageCount;
        uint32_t imageCount = glm::clamp(createInfo.desiredImageCount, minImageCount, maxImageCount);
        uint32_t queueFamilyIndices[] = { m_queueIndexGraphics, m_queueIndexPresent };

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.surface = m_surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_ASSERT_RESULT_CTX(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain), "failed to create swap chain!");

        m_format = surfaceFormat.format;
        m_extent = extent;

        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_images.data());
 
        m_imageViews.resize(m_images.size());

        for (size_t i = 0; i < m_imageViews.size(); ++i)
        {
            VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            imageViewCreateInfo.image = m_images[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_format;
            imageViewCreateInfo.components = VkComponentMapping{};
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;
            m_imageViews[i] = CreateRef<VulkanImageView>(m_device, imageViewCreateInfo);
        }

        m_imageAvailableSignals.resize(m_maxFramesInFlight);

        for (auto& semaphore : m_imageAvailableSignals)
        {
            semaphore = CreateRef<VulkanSemaphore>(m_device);
        }

        m_outofdate = false;

        PK_LOG_VERBOSE("Swapchain rebuilt! (%i,%i)", m_extent.width, m_extent.height);
    }

    void VulkanSwapchain::Release()
    {
        m_imageAvailableSignals.clear();
        m_imageViews.clear();

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
    }

    bool VulkanSwapchain::TryAcquireNextImage(VulkanSemaphore** imageAvailableSignal)
    {
        if (m_outofdate)
        {
            Rebuild(m_cachedCreateInfo);
            glfwWaitEvents();
        }

        auto result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSignals[m_frameIndex]->vulkanSemaphore, VK_NULL_HANDLE, &m_imageIndex);
        *imageAvailableSignal = m_imageAvailableSignals[m_frameIndex].get();

        PK_THROW_ASSERT(SwapchainErrorAssert(result, m_outofdate, m_suboptimal), "Failed to acquire swap chain image!");

        if (!m_outofdate)
        {
            ++m_frameIndex;
            m_frameIndex %= m_maxFramesInFlight;
        }

        return !m_outofdate;
    }

    void VulkanSwapchain::Present(VulkanSemaphore* waitSignal)
    {
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSignal->vulkanSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_imageIndex;
        presentInfo.pResults = nullptr; // Optional
        auto result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

        PK_THROW_ASSERT(SwapchainErrorAssert(result, m_outofdate, m_suboptimal), "Failed to present swap chain image!");
    }

    void VulkanSwapchain::OnWindowResize(int w, int h)
    {
        m_cachedCreateInfo.desiredExtent.width = (uint32_t)w;
        m_cachedCreateInfo.desiredExtent.height = (uint32_t)h;
        m_outofdate = true;
    }
}