#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FileIOBMP.h"
#include "VulkanWindow.h"

namespace PK
{
    template<typename T, typename ... Args>
    static void SafeInvokeFunction(const T& function, const Args& ... args)
    {
        if (function)
        {
            function(args...);
        }
    }

    VulkanWindow::VulkanWindow(VulkanDriver* driver, const WindowDescriptor& descriptor) : m_driver(driver)
    {
        PlatformWindowDescriptor platformDescriptor;
        platformDescriptor.title = descriptor.title.c_str();
        platformDescriptor.position = PK_INT2_MINUS_ONE;
        platformDescriptor.size = descriptor.size;
        platformDescriptor.sizemin = { MIN_SIZE, MIN_SIZE };
        platformDescriptor.sizemax = PK_INT2_MINUS_ONE;
        platformDescriptor.isVisible = true;
        platformDescriptor.isResizable = true;
        platformDescriptor.isFloating = false;
        platformDescriptor.useDpiScaling = false;
        platformDescriptor.activateOnFirstShow = true;
        m_window = Platform::CreateWindow(platformDescriptor);
        PK_THROW_ASSERT(m_window, "Failed To Create Window");

        m_window->SetListener(this);

        if (descriptor.iconPath.Length() > 0)
        {
            auto iconWidth = 0;
            auto iconHeight = 0;
            uint8_t* pixels = nullptr;
            int32_t iconBytesPerPixel = 0;
            FileIO::ReadBMP(descriptor.iconPath.c_str(), &pixels, &iconWidth, &iconHeight, &iconBytesPerPixel);
            PK_THROW_ASSERT(iconBytesPerPixel == 4, "Trying to load an icon with invalid bytes per pixel value, %i", iconBytesPerPixel);
            m_window->SetIcon(pixels, { iconWidth, iconHeight });
            free(pixels);
        }

        VK_ASSERT_RESULT_CTX(VulkanCreateSurfaceKHR(m_driver->instance, m_window->GetNativeWindowHandle(), &m_surface), "Failed to create window surface!");

        VkBool32 presentSupported;
        VK_ASSERT_RESULT_CTX(vkGetPhysicalDeviceSurfaceSupportKHR(m_driver->physicalDevice, m_driver->queues->GetQueue(QueueType::Present)->GetFamily(), m_surface, &presentSupported), "Surface support query failure!");
        PK_THROW_ASSERT(presentSupported, "Surface present not supported on selected physical device!");

        SwapchainCreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.desiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.desiredExtent = VkExtent2D{ descriptor.size.x, descriptor.size.y };
        swapchainCreateInfo.desiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
        swapchainCreateInfo.desiredImageCount = 4; // More images yields faster release of next image by present. But causes some instability.
        swapchainCreateInfo.desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainCreateInfo.maxFramesInFlight = PK_RHI_MAX_FRAMES_IN_FLIGHT;
        swapchainCreateInfo.nativeMonitor = nullptr;

        m_swapchain = CreateUnique<VulkanSwapchain>(m_driver->physicalDevice,
            m_driver->device,
            m_surface,
            m_driver->queues->GetQueue(QueueType::Graphics),
            m_driver->queues->GetQueue(QueueType::Present),
            swapchainCreateInfo);

        SetCursorVisible(descriptor.cursorVisible);
    }

    VulkanWindow::~VulkanWindow()
    {
        m_driver->WaitForIdle();

        m_swapchain = nullptr;

        vkDestroySurfaceKHR(m_driver->instance, m_surface, nullptr);

        if (m_window)
        {
            Platform::DestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    void VulkanWindow::Begin()
    {
        if (m_isFullScreen != m_swapchain->IsExclusiveFullScreen())
        {        
            m_window->SetFullScreen(m_isFullScreen);
        }

        while (!m_swapchain->TryAcquireNextImage(&m_imageAvailableSignal))
        {
            Platform::WaitEvents();
        }

        m_inWindowScope = true;
    }

    void VulkanWindow::End()
    {
        PK_THROW_ASSERT(m_inWindowScope, "Trying to end a frame that outside of a frame scope!")

        VkSemaphore renderingFinishedSignal = VK_NULL_HANDLE;

        // Window write is expected to be in the last (and implicit) graphics submit.
        m_driver->queues->GetQueue(QueueType::Graphics)->QueueWait(m_imageAvailableSignal, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        m_driver->queues->GetQueue(QueueType::Graphics)->commandPool->GetCurrent()->ValidateWindowPresent(this);
        m_driver->queues->SubmitCurrent(QueueType::Graphics, &renderingFinishedSignal);
        m_swapchain->Present(renderingFinishedSignal);
        m_inWindowScope = false;
    }

    void VulkanWindow::SetFrameFence(const FenceRef& fence)
    {
        m_swapchain->SetFrameFence(fence);
    }

    void VulkanWindow::SetCursorVisible(bool value)
    {
        m_window->SetCursorLock(false, value);
    }

    bool VulkanWindow::IPlatformWindow_OnEvent([[maybe_unused]] PlatformWindow* window, PlatformWindowEvent evt)
    {
        switch (evt)
        {
            case PlatformWindowEvent::FullScreenRequest:
            {
                const auto nativeMonitor = m_window->GetNativeMonitorHandle();
                const auto acquiredFullSCreen = m_swapchain->TrySetFullScreen(nativeMonitor);

                if (!acquiredFullSCreen)
                {
                    m_isFullScreen = false;
                }

                return acquiredFullSCreen;
            }
            case PlatformWindowEvent::FullScreenExit:
            {
                m_swapchain->TrySetFullScreen(nullptr);
                m_isFullScreen = false;
                break;
            }

            case PlatformWindowEvent::Close:
            {
                SafeInvokeFunction(m_onClose);
                break;
            }
            case PlatformWindowEvent::Resize:
            {
                auto size = m_window->GetSize();
                m_swapchain->SetDesiredExtent({ (uint32_t)size.x, (uint32_t)size.y });
                SafeInvokeFunction(m_onResize, size.x, size.y);
                break;
            }

            default:
                break;
        }

        return false;
    }
}