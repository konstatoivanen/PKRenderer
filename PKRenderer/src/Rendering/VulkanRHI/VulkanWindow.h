#pragma once
#include "PrecompiledHeader.h"
#include "Core/Window.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Systems/VulkanSwapchain.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI
{
    using namespace Systems;

    class VulkanWindow : public PK::Core::Window
    {
        public:
            VulkanWindow(VulkanDriver* driver, const PK::Core::WindowProperties& properties);
            ~VulkanWindow();

            uint3 GetResolution() const override { return m_swapchain->GetResolution(); }
            float GetAspectRatio() const override { return m_swapchain->GetAspectRatio(); }
            bool IsAlive() const override { return m_alive; }
            bool IsMinimized() const override { return m_minimized; }
            bool IsVSync() const override { return m_vsync; }

            void Begin() override;
            void End() override;
            void SetCursorVisible(bool value) override;
            void SetVSync(bool enabled) override { m_vsync = enabled; };
            inline void PollEvents() const override { glfwPollEvents(); }
            inline void* GetNativeWindow() const { return m_window; }

            constexpr VkRect2D GetRect() const { return m_swapchain->GetRect(); }
            constexpr VkExtent2D GetExtent() const { return m_swapchain->GetExtent(); }
            constexpr VkFormat GetNativeFormat() const { return m_swapchain->GetNativeFormat(); }
            const VulkanRenderTarget GetRenderTarget() const { return m_swapchain->GetRenderTarget(); }

        private:
            const VulkanDriver* m_driver;
            VkSurfaceKHR m_surface;
            GLFWwindow* m_window;
            Scope<VulkanSwapchain> m_swapchain;
            VulkanSemaphore* m_imageAvailableSignal;

            bool m_vsync = true;
            bool m_cursorVisible = true;
            bool m_alive = true;
            bool m_minimized = false;
    };
}