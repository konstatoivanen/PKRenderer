#pragma once
#include "Core/Platform/PlatformInterfaces.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanSwapchain.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"

namespace PK
{
    class VulkanWindow : public RHIWindow, public IPlatformWindowListener
    {
        public:
            VulkanWindow(VulkanDriver* driver, const WindowDescriptor& descriptor);
            ~VulkanWindow();

            uint3 GetResolution() const final { return m_swapchain->GetResolution(); }
            float GetAspectRatio() const final { return m_swapchain->GetAspectRatio(); }
            bool IsClosing() const final { return m_window->IsClosing(); }
            bool IsMinimized() const final { return m_window->IsMinimized(); }
            bool IsVSync() const final { return m_vsync; }
            bool IsFullscreen() const final { return m_isFullScreen; }

            void Begin() final;
            void End() final;
            void SetFrameFence(const FenceRef& fence) final;
            void SetCursorVisible(bool value) final;
            void SetVSync(bool value) final { m_vsync = value; };
            void SetFullscreen(bool value) final { m_isFullScreen = value; }
            void SetOnResizeCallback(const std::function<void(int width, int height)>& callback) final { m_onResize = callback; }
            void SetOnCloseCallback(const std::function<void()>& callback) final { m_onClose = callback; }
            PlatformWindow* GetNativeWindow() const final { return m_window; }

            uint4 GetRect() const final { return m_swapchain->GetRect(); }
            constexpr VkExtent2D GetExtent() const { return m_swapchain->GetExtent(); }
            constexpr VkFormat GetNativeFormat() const { return m_swapchain->GetNativeFormat(); }
            const VulkanBindHandle* GetBindHandle() const { return m_swapchain->GetBindHandle(); }
            
            bool IPlatformWindow_OnEvent(PlatformWindow* window, PlatformWindowEvent evt) final;

        private:
            const VulkanDriver* m_driver;
            VkSemaphore m_imageAvailableSignal = VK_NULL_HANDLE;
            VkSurfaceKHR m_surface;
            PlatformWindow* m_window;
            Unique<VulkanSwapchain> m_swapchain;

            std::function<void(int width, int height)> m_onResize;
            std::function<void()> m_onClose;

            bool m_inWindowScope = false;
            bool m_vsync = true;
            bool m_isFullScreen = false;
    };
}