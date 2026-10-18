#pragma once
#include "Core/RHI/Vulkan/VulkanDriver.h"

namespace PK
{
    struct VulkanQueue;

    struct VulkanSwapchain : public RHISwapchain
    {
        VulkanSwapchain(VulkanDriver* driver, const SwapchainDescriptor& descriptor);
        ~VulkanSwapchain();

        void SetDesiredResolution(const uint2& resolution) final;
        void SetDesiredFormat(TextureFormat format) final;
        void SetDesiredColorSpace(ColorSpace colorSpace) final;
        void SetDesiredVSyncMode(VSyncMode vsyncMode) final;
        bool AcquireFullScreen(const void* nativeMonitor) final;
        bool IsFullScreen() const final { return m_descriptor.nativeMonitorHandle != nullptr; }
        uint3 GetResolution() const final { return { m_extent.width, m_extent.height, 1 }; }
        TextureFormat GetFormat() const final { return VulkanEnumConvert::GetTextureFormat(m_format.format); }
        ColorSpace GetColorSpace() const final { return VulkanEnumConvert::GetColorSpace(m_format.colorSpace); }
        VSyncMode GetVSyncMode() const final { return VulkanEnumConvert::GetVSyncMode(m_presentMode); }

        void WaitForPresent(uint32_t historyOffset, uint64_t timeoutNanos);
        void Release();
        void Rebuild(const SwapchainDescriptor& createInfo);

        const VulkanResourceView* GetResourceView() const { return &m_imageViews[m_imageIndex]->resourceView; }
        const VulkanImageView* GetImageView() const { return m_imageViews[m_imageIndex]; }
        VkSemaphore ConsumeAcquireSignal();
        VkSemaphore ConsumePresentSignal();

        void Validate();
        void AcquireNextImage();
        void Present(VulkanQueue* queue, VkSemaphore presentSignal);

    private:
        const VulkanDriver* m_driver = nullptr;

        SwapchainDescriptor m_descriptor{};
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkImage m_images[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
        VulkanImageView* m_imageViews[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
        VkSemaphore m_semaphoresAcquire[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
        VkSemaphore m_semaphoresPresent[PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT]{};
        VkPresentModeKHR m_presentModes[(uint32_t)VSyncMode::EnumCount]{};
        VkPresentModeKHR m_presentMode;
        VkSurfaceFormatKHR m_format;
        VkExtent2D m_extent;
        uint32_t m_presentModeCount;
        uint32_t m_imageCount;

        VkSemaphore m_acquireSignal = VK_NULL_HANDLE;
        VkSemaphore m_presentSignal = VK_NULL_HANDLE;

        uint32_t m_frameIndex = 0u;
        uint32_t m_imageIndex = 0u;
        uint64_t m_presentId = 0ull;
        bool m_outofdate = false;
        bool m_hasExternalFrameFence = false;
    };
}
