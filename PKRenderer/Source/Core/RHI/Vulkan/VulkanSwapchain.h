#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanQueue;

    struct SwapchainCreateInfo
    {
        VkExtent2D desiredExtent;
        VkFormat desiredFormat;
        VkColorSpaceKHR desiredColorSpace;
        VkPresentModeKHR desiredPresentMode;
        uint32_t desiredImageCount;
        uint32_t maxFramesInFlight;
        const void* nativeMonitor;
        bool exclusiveFullScreen;
    };

    class VulkanSwapchain : public NoCopy
    {
        public:
            // Max count so that resources can use static arrays.
            constexpr static uint32_t MaxImageCount = 16u;

            VulkanSwapchain(const VkPhysicalDevice physicalDevice, 
                            const VkDevice device, 
                            const VkSurfaceKHR surface,
                            VulkanQueue* queueGraphics,
                            VulkanQueue* queuePresent,
                            const SwapchainCreateInfo& createInfo);

            ~VulkanSwapchain() { Release(); }

            void Rebuild(const SwapchainCreateInfo& createInfo);
            void Release();

            void SetDesiredExtent(const VkExtent2D& extent);
            void RequestExclusiveFullScreen(const void* nativeMonitor, bool value);
            void SetFrameFence(const FenceRef& fence);
            bool TryAcquireNextImage(VkSemaphore* imageAvailableSignal);
            void Present(VkSemaphore waitSignal);

            const VulkanBindHandle* GetBindHandle() const { return &m_imageViews[m_imageIndex]->bindHandle; }
            const VulkanImageView* GetImageView() const { return m_imageViews[m_imageIndex]; }
            constexpr VkExtent2D GetExtent() const { return m_extent; }
            constexpr uint3 GetResolution() const { return { m_extent.width, m_extent.height, 1 }; }
            constexpr uint32_t GetWidth() const { return m_extent.width; }
            constexpr uint32_t GetHeight() const { return m_extent.height; }
            constexpr float GetAspectRatio() const { return (float)m_extent.width / (float)m_extent.height; }
            constexpr uint4 GetRect() const { return { 0, 0, m_extent.width, m_extent.height }; }
            constexpr VkFormat GetNativeFormat() const { return m_format; }
            constexpr bool IsExclusiveFullScreen() const { return m_isExlclusiveFullScreen; }

        private:
            const VkPhysicalDevice m_physicalDevice;
            const VkDevice m_device;
            const VkSurfaceKHR m_surface;
            VulkanQueue* m_queueGraphics;
            VulkanQueue* m_queuePresent;

            SwapchainCreateInfo m_cachedCreateInfo;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            VkImage m_images[MaxImageCount]{};
            VulkanImageView* m_imageViews[MaxImageCount]{};
            FenceRef m_frameFences[MaxImageCount]{};
            uint32_t m_imageCount;
            VkFormat m_format;
            VkExtent2D m_extent;
            uint32_t m_maxFramesInFlight;
            uint32_t m_frameIndex = 0u;
            uint32_t m_imageIndex = 0u;
            bool m_isExlclusiveFullScreen = false;
            bool m_outofdate = false;
            bool m_suboptimal = false;
            bool m_hasExternalFrameFence = false;
    };
}
