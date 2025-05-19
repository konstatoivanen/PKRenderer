#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FileIOBMP.h"
#include "Core/RHI/RHInterfaces.h"
#include "Window.h"

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

    Window::Window(const WindowDescriptor& descriptor)
    {
        PlatformWindowDescriptor platformDescriptor;
        platformDescriptor.title = descriptor.title.c_str();
        platformDescriptor.position = descriptor.position;
        platformDescriptor.size = descriptor.size;
        platformDescriptor.sizemin = { MIN_SIZE, MIN_SIZE };
        platformDescriptor.sizemax = descriptor.sizemax;
        platformDescriptor.isVisible = descriptor.visible;
        platformDescriptor.isResizable = descriptor.resizable;
        platformDescriptor.isFloating = descriptor.floating;
        platformDescriptor.useDpiScaling = descriptor.dpiScaling;
        platformDescriptor.autoActivate = descriptor.autoActivate;
        m_native = Platform::CreateWindow(platformDescriptor);
        PK_THROW_ASSERT(m_native, "Failed To Create Window");

        m_native->SetListener(this);

        if (descriptor.iconPath.Length() > 0)
        {
            auto iconWidth = 0;
            auto iconHeight = 0;
            uint8_t* pixels = nullptr;
            int32_t iconBytesPerPixel = 0;
            FileIO::ReadBMP(descriptor.iconPath.c_str(), &pixels, &iconWidth, &iconHeight, &iconBytesPerPixel);
            PK_THROW_ASSERT(iconBytesPerPixel == 4, "Trying to load an icon with invalid bytes per pixel value, %i", iconBytesPerPixel);
            m_native->SetIcon(pixels, { iconWidth, iconHeight });
            free(pixels);
        }

        // @TODO HDR Support?!?
        SwapchainDescriptor swapchainDescriptor{};
        swapchainDescriptor.desiredResolution = descriptor.size;
        swapchainDescriptor.desiredFormat = TextureFormat::BGRA8;
        swapchainDescriptor.desiredColorSpace = ColorSpace::sRGB_NonLinear;
        swapchainDescriptor.desiredVSyncMode = descriptor.vsync;
        swapchainDescriptor.nativeWindowHandle = m_native->GetNativeWindowHandle();
        swapchainDescriptor.nativeMonitorHandle = nullptr;
        m_swapchain = RHI::CreateSwapchain(swapchainDescriptor);
    }

    Window::~Window()
    {
        m_swapchain = nullptr;

        if (m_native)
        {
            Platform::DestroyWindow(m_native);
            m_native = nullptr;
        }
    }


    uint3 Window::GetResolution() const 
    { 
        return m_swapchain->GetResolution(); 
    }

    float Window::GetAspectRatio() const 
    { 
        return m_swapchain->GetAspectRatio(); 
    }

    void Window::SetFrameFence(const FenceRef& fence) 
    { 
        m_swapchain->SetFrameFence(fence); 
    }

    void Window::SetVSync(VSyncMode value) 
    {
        m_vsync = value; 
        m_swapchain->SetDesiredVSyncMode(value);
    };


    void Window::Begin()
    {
        if (m_isFullScreen != m_swapchain->IsFullScreen())
        {
            m_native->SetFullScreen(m_isFullScreen);
        }

        while (!m_swapchain->AcquireNextImage())
        {
            Platform::WaitEvents();
        }

        m_inWindowScope = true;
    }

    void Window::End()
    {
        PK_THROW_ASSERT(m_inWindowScope, "Trying to end a frame that outside of a frame scope!")
        m_swapchain->Present();
        m_inWindowScope = false;
    }

    bool Window::IPlatformWindow_OnEvent([[maybe_unused]] PlatformWindow* window, PlatformWindowEvent evt)
    {
        switch (evt)
        {
            case PlatformWindowEvent::FullScreenRequest:
            {
                const auto nativeMonitor = m_native->GetNativeMonitorHandle();
                m_isFullScreen = m_swapchain->AcquireFullScreen(nativeMonitor);
                return m_isFullScreen;
            }
            case PlatformWindowEvent::FullScreenExit:
            {
                m_swapchain->AcquireFullScreen(nullptr);
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
                auto size = m_native->GetSize();
                m_swapchain->SetDesiredResolution(size);
                SafeInvokeFunction(m_onResize, size.x, size.y);
                break;
            }

            default: break;
        }

        return false;
    }
}
