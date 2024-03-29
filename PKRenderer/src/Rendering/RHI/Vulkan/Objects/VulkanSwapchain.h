#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanQueue.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    struct SwapchainCreateInfo
    {
        VkExtent2D desiredExtent;
        VkFormat desiredFormat;
        VkColorSpaceKHR desiredColorSpace;
        VkPresentModeKHR desiredPresentMode;
        uint32_t desiredImageCount;
        uint32_t maxFramesInFlight;
    };

    class VulkanSwapchain : public PK::Utilities::NoCopy
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

            void SetFrameFence(const FenceRef& fence);
            bool TryAcquireNextImage(VkSemaphore* imageAvailableSignal);
            void Present(VkSemaphore waitSignal);
            void OnWindowResize(int w, int h);

            const VulkanBindHandle* GetBindHandle() const { return &m_bindHandles[m_imageIndex]; }
            const VulkanImageView* GetImageView() const { return m_imageViews[m_imageIndex]; }
            constexpr VkExtent2D GetExtent() const { return m_extent; }
            constexpr Math::uint3 GetResolution() const { return { m_extent.width, m_extent.height, 1 }; }
            constexpr uint32_t GetWidth() const { return m_extent.width; }
            constexpr uint32_t GetHeight() const { return m_extent.height; }
            constexpr float GetAspectRatio() const { return (float)m_extent.width / (float)m_extent.height; }
            constexpr Math::uint4 GetRect() const { return { 0, 0, m_extent.width, m_extent.height }; }
            constexpr VkFormat GetNativeFormat() const { return m_format; }

        private:
            const VkPhysicalDevice m_physicalDevice;
            const VkDevice m_device;
            const VkSurfaceKHR m_surface;
            VulkanQueue* m_queueGraphics;
            VulkanQueue* m_queuePresent;

            SwapchainCreateInfo m_cachedCreateInfo;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            VkImage m_images[MaxImageCount];
            VulkanImageView* m_imageViews[MaxImageCount];
            FenceRef m_frameFences[MaxImageCount];
            VulkanBindHandle m_bindHandles[MaxImageCount];
            uint32_t m_imageCount;
            VkFormat m_format;
            VkExtent2D m_extent;
            uint32_t m_maxFramesInFlight;
            uint32_t m_frameIndex = 0u;
            uint32_t m_imageIndex = 0u;
            bool m_outofdate = false;
            bool m_suboptimal = false;
            bool m_hasExternalFrameFence = false;
    };
}
