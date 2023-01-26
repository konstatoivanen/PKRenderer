#include "PrecompiledHeader.h"
#include "VulkanWindow.h"
#include "Core/Services/Log.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Utilities;
    using namespace Services;
    using namespace Objects;
    using namespace Structs;

    static VulkanWindow* GetWindowPtr(GLFWwindow* window)
    {
        return (VulkanWindow*)glfwGetWindowUserPointer(window);
    }

    template<typename T, typename ... Args>
    static void SafeInvokeFunction(const T& function, const Args& ... args)
    {
        if (function)
        {
            function(args...);
        }
    }

    VulkanWindow::VulkanWindow(VulkanDriver* driver, const PK::Core::WindowProperties& properties) :
        m_driver(driver)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        m_window = glfwCreateWindow(properties.width, properties.height, properties.title.c_str(), nullptr, nullptr);
        PK_THROW_ASSERT(m_window, "Failed To Create Window");

        glfwSetWindowUserPointer(m_window, this);

        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* nativeWindow, int width, int height)
        {
            auto window = GetWindowPtr(nativeWindow);
            window->m_minimized = width == 0 || height == 0;
            window->m_swapchain->OnWindowResize(width, height);
            SafeInvokeFunction(window->OnResize, width, height);
        });
        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* nativeWindow)
        {
            auto window = GetWindowPtr(nativeWindow);
            window->m_alive = false;
            SafeInvokeFunction(window->OnClose);
        });
        glfwSetKeyCallback(m_window, [](GLFWwindow* nativeWindow, int key, int scancode, int action, int mods) { SafeInvokeFunction(GetWindowPtr(nativeWindow)->OnKeyInput, key, scancode, action, mods); });
        glfwSetCharCallback(m_window, [](GLFWwindow* nativeWindow, uint32_t keycode) { SafeInvokeFunction(GetWindowPtr(nativeWindow)->OnCharInput, keycode); });
        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* nativeWindow, int button, int action, int mods) { SafeInvokeFunction(GetWindowPtr(nativeWindow)->OnMouseButtonInput, button, action, mods); });
        glfwSetScrollCallback(m_window, [](GLFWwindow* nativeWindow, double xOffset, double yOffset) { SafeInvokeFunction(GetWindowPtr(nativeWindow)->OnScrollInput, xOffset, yOffset); });
        glfwSetCursorPosCallback(m_window, [](GLFWwindow* nativeWindow, double xPos, double yPos) {SafeInvokeFunction(GetWindowPtr(nativeWindow)->OnCursorInput, xPos, yPos); });
        glfwSetErrorCallback([](int error, const char* description) { PK_THROW_ERROR("GLFW Error (%i) : %s", error, description); });
        VK_ASSERT_RESULT_CTX(glfwCreateWindowSurface(m_driver->instance, m_window, nullptr, &m_surface), "Failed to create window surface!");

        VkBool32 presentSupported;
        VK_ASSERT_RESULT_CTX(vkGetPhysicalDeviceSurfaceSupportKHR(m_driver->physicalDevice, m_driver->queueFamilies[QueueType::Present], m_surface, &presentSupported), "Surface support query failure!");
        PK_THROW_ASSERT(presentSupported, "Surface present not supported on selected physical device!");

        SwapchainCreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.desiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.desiredExtent = VkExtent2D{ properties.width, properties.height };
        swapchainCreateInfo.desiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
        swapchainCreateInfo.desiredImageCount = 3;
        swapchainCreateInfo.desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainCreateInfo.maxFramesInFlight = PK_MAX_FRAMES_IN_FLIGHT;
        m_swapchain = CreateScope<VulkanSwapchain>(m_driver->physicalDevice,
                                                   m_driver->device,
                                                   m_surface,
                                                   m_driver->queueFamilies[QueueType::Graphics],
                                                   m_driver->queueFamilies[QueueType::Present],
                                                   swapchainCreateInfo);

        SetCursorVisible(properties.cursorVisible);
    }
    
    VulkanWindow::~VulkanWindow()
    {
        m_driver->WaitForIdle();

        m_swapchain = nullptr;

        vkDestroySurfaceKHR(m_driver->instance, m_surface, nullptr);

        if (m_window)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
    }
    
    void VulkanWindow::Begin()
    {
        while (!m_swapchain->TryAcquireNextImage(&m_imageAvailableSignal))
        {
            PollEvents();
        }
    }

    void VulkanWindow::End()
    {
        PK_THROW_ASSERT(m_imageAvailableSignal != nullptr, "Trying to end a frame that outside of a frame scope!")
        auto frameFence = m_driver->commandBufferPool->GetCurrent()->fence->vulkanFence;
        auto frameGate = m_driver->commandBufferPool->GetCurrent()->GetOnCompleteGate();

        m_driver->commandBufferPool->SubmitCurrent(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_imageAvailableSignal);
        m_swapchain->Present(m_driver->commandBufferPool->AcquireRenderingFinishedSignal(), frameFence, frameGate);
        m_imageAvailableSignal = nullptr;
    }

    void VulkanWindow::SetCursorVisible(bool value)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, value ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
        m_cursorVisible = value;
    }
}