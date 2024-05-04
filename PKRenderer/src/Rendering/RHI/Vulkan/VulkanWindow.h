#pragma once
#include <gfx.h>
#include "Rendering/RHI/Window.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanSwapchain.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"

namespace PK::Rendering::RHI::Vulkan
{
    class VulkanWindow : public Rendering::RHI::Window
    {
        public:
            using PK::Rendering::RHI::Window::GetRect;

            VulkanWindow(VulkanDriver* driver, const WindowProperties& properties);
            ~VulkanWindow();

            Math::uint3 GetResolution() const final { return m_swapchain->GetResolution(); }
            float GetAspectRatio() const final { return m_swapchain->GetAspectRatio(); }
            bool IsAlive() const final { return m_alive; }
            bool IsMinimized() const final { return m_minimized; }
            bool IsVSync() const final { return m_vsync; }

            void Begin() final;
            void End() final;
            void SetFrameFence(const PK::Utilities::FenceRef& fence) final;
            void SetCursorVisible(bool value) final;
            void SetVSync(bool enabled) final { m_vsync = enabled; };
            inline void PollEvents() const final { glfwPollEvents(); }
            inline void WaitEvents() const final { glfwWaitEvents(); }
            void* GetNativeWindow() const final { return m_window; }

            Math::uint4 GetRect() const final { return m_swapchain->GetRect(); }
            constexpr VkExtent2D GetExtent() const { return m_swapchain->GetExtent(); }
            constexpr VkFormat GetNativeFormat() const { return m_swapchain->GetNativeFormat(); }
            const VulkanBindHandle* GetBindHandle() const { return m_swapchain->GetBindHandle(); }

        private:
            const VulkanDriver* m_driver;
            VkSemaphore m_imageAvailableSignal = VK_NULL_HANDLE;
            VkSurfaceKHR m_surface;
            GLFWwindow* m_window;
            PK::Utilities::Scope<Objects::VulkanSwapchain> m_swapchain;

            bool m_inWindowScope = false;
            bool m_vsync = true;
            bool m_cursorVisible = true;
            bool m_alive = true;
            bool m_minimized = false;
    };
}