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

            Math::uint3 GetResolution() const override { return m_swapchain->GetResolution(); }
            float GetAspectRatio() const override { return m_swapchain->GetAspectRatio(); }
            bool IsAlive() const override { return m_alive; }
            bool IsMinimized() const override { return m_minimized; }
            bool IsVSync() const override { return m_vsync; }

            void Begin() override final;
            void End() override final;
            void SetCursorVisible(bool value) override final;
            void SetVSync(bool enabled) override final { m_vsync = enabled; };
            inline void PollEvents() const override final { glfwPollEvents(); }
            inline void WaitEvents() const override final { glfwWaitEvents(); }
            void* GetNativeWindow() const override final { return m_window; }

            Math::uint4 GetRect() const override final { return m_swapchain->GetRect(); }
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