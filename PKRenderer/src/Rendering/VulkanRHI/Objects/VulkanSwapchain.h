#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::Objects
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
            VulkanSwapchain(const VkPhysicalDevice physicalDevice, 
                            const VkDevice device, 
                            const VkSurfaceKHR surface,
                            uint32_t queueIndexGraphics, 
                            uint32_t queueIndexPresent,
                            const SwapchainCreateInfo& createInfo);

            ~VulkanSwapchain() { Release(); }

            void Rebuild(const SwapchainCreateInfo& createInfo);
            void Release();

            bool TryAcquireNextImage(VulkanSemaphore** imageAvailableSignal);
            void Present(VulkanSemaphore* waitSignal, VkFence frameFence, const Structs::ExecutionGate& gate);
            void OnWindowResize(int w, int h);

            const VulkanBindHandle* GetBindHandle() const { return &m_bindHandles[m_imageIndex]; }
            const VulkanImageView* GetImageView() const { return m_imageViews.at(m_imageIndex); }
            constexpr VkExtent2D GetExtent() const { return m_extent; }
            constexpr Math::uint3 GetResolution() const { return { m_extent.width, m_extent.height, 1 }; }
            constexpr float GetAspectRatio() const { return (float)m_extent.width / (float)m_extent.height; }
            constexpr Math::uint4 GetRect() const { return { 0, 0, m_extent.width, m_extent.height }; }
            constexpr VkFormat GetNativeFormat() const { return m_format; }

        private:
            struct FrameFence
            {
                Structs::ExecutionGate gate;
                VkFence fence;
            };

            const VkPhysicalDevice m_physicalDevice;
            const VkDevice m_device;
            const VkSurfaceKHR m_surface;
            const uint32_t m_queueIndexGraphics;
            const uint32_t m_queueIndexPresent;

            SwapchainCreateInfo m_cachedCreateInfo;
            VkQueue m_presentQueue = VK_NULL_HANDLE;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            std::vector<VkImage> m_images;
            std::vector<VulkanImageView*> m_imageViews;
            std::vector<VulkanSemaphore*> m_imageAvailableSignals;
            std::vector<FrameFence> m_frameFences;
            VulkanBindHandle* m_bindHandles;
            VkFormat m_format;
            VkExtent2D m_extent;
            uint32_t m_maxFramesInFlight;
            uint32_t m_frameIndex;
            uint32_t m_imageIndex;
            bool m_outofdate;
            bool m_suboptimal;
    };
}