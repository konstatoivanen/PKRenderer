#include "PrecompiledHeader.h"
#include "VulkanSwapchain.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Utilities;

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
        VulkanQueue* queueGraphics,
        VulkanQueue* queuePresent,
        const SwapchainCreateInfo& createInfo) :
        m_physicalDevice(physicalDevice),
        m_device(device),
        m_surface(surface),
        m_queueGraphics(queueGraphics),
        m_queuePresent(queuePresent)

    {
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

        auto maxImageCount = capabilities.maxImageCount > 0 ? capabilities.maxImageCount : UINT32_MAX;
        auto minImageCount = capabilities.minImageCount;
        maxImageCount = maxImageCount > MaxImageCount ? MaxImageCount : maxImageCount;

        m_imageCount = glm::clamp(createInfo.desiredImageCount, minImageCount, maxImageCount);
        uint32_t queueFamilyIndices[] = { m_queueGraphics->GetFamily(), m_queuePresent->GetFamily() };

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.surface = m_surface;
        swapchainCreateInfo.minImageCount = m_imageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
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
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_ASSERT_RESULT_CTX(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain), "failed to create swap chain!");

        m_format = surfaceFormat.format;
        m_extent = extent;

        vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_images);

        for (size_t i = 0u; i < m_imageCount; ++i)
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
            m_imageViews[i] = GraphicsAPI::GetActiveDriver<VulkanDriver>()->imageViewPool.New(m_device, imageViewCreateInfo, (std::string("Swapchain.Image") + std::to_string(i)).c_str());
        }

        for (size_t i = 0u; i < m_imageCount; ++i)
        {
            auto handle = &m_bindHandles[i];
            handle->image.image = m_images[i];
            handle->image.alias = VK_NULL_HANDLE;
            handle->image.view = m_imageViews[i]->view;
            handle->image.sampler = VK_NULL_HANDLE;
            handle->image.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            handle->image.format = m_format;
            handle->image.extent = { m_extent.width, m_extent.height, 1 };
            handle->image.range = { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u };
            handle->image.samples = 1u;
            handle->IncrementVersion();
        }

        for (auto& fence : m_frameFences)
        {
            fence.Invalidate();
        }

        m_outofdate = false;

        PK_LOG_VERBOSE("Swapchain rebuilt! (%i,%i)", m_extent.width, m_extent.height);
    }

    void VulkanSwapchain::Release()
    {
        for (size_t i = 0u; i < MaxImageCount; ++i)
        {
            if (m_imageViews[i] != nullptr)
            {
                GraphicsAPI::GetActiveDriver<VulkanDriver>()->imageViewPool.Delete(m_imageViews[i]);
                m_imageViews[i] = nullptr;
            }
        }

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
    }

    void VulkanSwapchain::SetFrameFence(const FenceRef& fence)
    {
        m_frameFences[m_frameIndex] = fence;
        m_hasExternalFrameFence = true;
    }

    bool VulkanSwapchain::TryAcquireNextImage(VkSemaphore* imageAvailableSignal)
    {
        if (m_outofdate)
        {
            Rebuild(m_cachedCreateInfo);
            glfwWaitEvents();
        }

        // This is not strictly necesssary when not using double buffering for staging buffers.
        // However, for them to have coherent memory operations we cannot push more than 2 frames at a time.
        PK_THROW_ASSERT(m_frameFences[m_frameIndex].WaitInvalidate(UINT64_MAX), "Frame fence timeout!");

        *imageAvailableSignal = m_queueGraphics->GetNextSemaphore();
        auto result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, *imageAvailableSignal, VK_NULL_HANDLE, &m_imageIndex);

        PK_THROW_ASSERT(SwapchainErrorAssert(result, m_outofdate, m_suboptimal), "Failed to acquire swap chain image!");

        return !m_outofdate;
    }

    void VulkanSwapchain::Present(VkSemaphore waitSignal)
    {
        // Frame synchronization for this frame is handled externally.
        if (!m_hasExternalFrameFence)
        {
            // Assumes previous submit was for rendering work.
            m_frameFences[m_frameIndex] = m_queueGraphics->GetFenceRef();
        }

        m_hasExternalFrameFence = false;
        m_frameIndex = (m_frameIndex + 1) % m_maxFramesInFlight;
        VK_ASSERT_RESULT(m_queuePresent->Present(m_swapchain, m_imageIndex, waitSignal));
    }

    void VulkanSwapchain::OnWindowResize(int w, int h)
    {
        m_cachedCreateInfo.desiredExtent.width = (uint32_t)w;
        m_cachedCreateInfo.desiredExtent.height = (uint32_t)h;
        m_outofdate = true;
    }
}