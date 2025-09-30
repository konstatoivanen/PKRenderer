#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"

namespace PK
{
    class VulkanQueue;

    class VulkanSwapchain : public RHISwapchain
    {
        public:
            VulkanSwapchain(VulkanDriver* driver, const SwapchainDescriptor& descriptor);

            ~VulkanSwapchain();

            void SetDesiredResolution(const uint2& resolution) final;
            void SetDesiredFormat(TextureFormat format) final;
            void SetDesiredColorSpace(ColorSpace colorSpace) final;
            void SetDesiredVSyncMode(VSyncMode vsyncMode) final;
            void SetFrameFence(const FenceRef& fence) final;
            bool AcquireFullScreen(const void* nativeMonitor) final;
            bool AcquireNextImage() final;
            void Present() final;
            bool IsFullScreen() const final { return m_descriptor.nativeMonitorHandle != nullptr; }
            uint3 GetResolution() const final { return { m_extent.width, m_extent.height, 1 }; }
            TextureFormat GetFormat() const final { return VulkanEnumConvert::GetTextureFormat(m_format.format); }
            ColorSpace GetColorSpace() const final { return VulkanEnumConvert::GetColorSpace(m_format.colorSpace); }
            VSyncMode GetVSyncMode() const final { return VulkanEnumConvert::GetVSyncMode(m_presentMode); }

            void Release();
            void Rebuild(const SwapchainDescriptor& createInfo);

            const VulkanBindHandle* GetBindHandle() const { return &m_imageViews[m_imageIndex]->bindHandle; }
            const VulkanImageView* GetImageView() const { return m_imageViews[m_imageIndex]; }
            VkSemaphore ConsumeImageSignal();

        private:
            const VulkanDriver* m_driver = nullptr;

            SwapchainDescriptor m_descriptor{};
            VkSemaphore m_imageSignal = VK_NULL_HANDLE;
            VkSurfaceKHR m_surface = VK_NULL_HANDLE;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            VkImage m_images[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
            VulkanImageView* m_imageViews[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
            FenceRef m_frameFences[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
            uint32_t m_imageCount;
            VkSurfaceFormatKHR m_format;
            VkPresentModeKHR m_presentMode;
            VkExtent2D m_extent;
            uint32_t m_frameIndex = 0u;
            uint32_t m_imageIndex = 0u;
            bool m_outofdate = false;
            bool m_suboptimal = false;
            bool m_hasExternalFrameFence = false;
    };
}
