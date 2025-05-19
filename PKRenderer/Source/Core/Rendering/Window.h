#pragma once
#include "Core/Platform/PlatformInterfaces.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/RHI.h"

namespace PK
{
    struct WindowDescriptor
    {
        FixedString64 title = "PK Window";
        FixedString256 iconPath = "";
        int2 position = PK_INT2_MINUS_ONE;
        int2 size = PK_INT2_ZERO;
        int2 sizemax = PK_INT2_MINUS_ONE;
        VSyncMode vsync = VSyncMode::Fifo;
        bool visible = false;
        bool resizable = false;
        bool floating = false;
        bool dpiScaling = false;
        bool autoActivate = false;
    };

    struct Window : public NoCopy, public IPlatformWindowListener
    {
        constexpr static uint32_t MIN_SIZE = 256u;

        Window(const WindowDescriptor& descriptor);
        ~Window();

        int4 GetRect() const { return m_native->GetRect(); }
        PlatformWindow* GetNative() const { return m_native; }
        RHISwapchain* GetSwapchain() const { return m_swapchain.get(); }
        VSyncMode GetVSyncMode() const { return m_vsync; }
        bool IsClosing() const { return m_native->IsClosing(); }
        bool IsMinimized() const { return m_native->IsMinimized(); }
        bool IsFullscreen() const { return m_isFullScreen; }
        void SetFullscreen(bool value) { m_isFullScreen = value; }
        void SetCursorLock(bool locked, bool visible) { m_native->SetCursorLock(locked, visible); }
        void SetOnResizeCallback(const std::function<void(int width, int height)>& callback) { m_onResize = callback; }
        void SetOnCloseCallback(const std::function<void()>& callback) { m_onClose = callback; }
        
        uint3 GetResolution() const;
        float GetAspectRatio() const;
        void SetFrameFence(const FenceRef& fence);
        void SetVSync(VSyncMode value);

        void Begin();
        void End();
        bool IPlatformWindow_OnEvent(PlatformWindow* window, PlatformWindowEvent evt) final;

    private:
        PlatformWindow* m_native;
        RHISwapchainScope m_swapchain;

        std::function<void(int width, int height)> m_onResize;
        std::function<void()> m_onClose;

        bool m_inWindowScope = false;
        bool m_isFullScreen = false;
        VSyncMode m_vsync = VSyncMode::Fifo;
    };
}
