#pragma once
#include "PrecompiledHeader.h"
#include "Core/Window.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanSwapchain.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI
{
    class VulkanWindow : public PK::Core::Window
    {
        public:
            using PK::Core::Window::GetRect;

            VulkanWindow(VulkanDriver* driver, const PK::Core::WindowProperties& properties);
            ~VulkanWindow();

            Math::uint3 GetResolution() const final { return m_swapchain->GetResolution(); }
            float GetAspectRatio() const final { return m_swapchain->GetAspectRatio(); }
            bool IsAlive() const final { return m_alive; }
            bool IsMinimized() const final { return m_minimized; }
            bool IsVSync() const final { return m_vsync; }

            void Begin() final;
            void End() final;
            void SetFrameFence(const Structs::FenceRef& fence) final;
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