#pragma once
#include "Core/Platform/Platform.h"
#include "Core/Utilities/Memory.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Input/InputDevice.h"
#include "Core/Math/Math.h"

namespace PK
{
    struct PlatformMemoryInfo
    {
        size_t physicalMemoryTotal;
        size_t physicalMemoryUsed;
        size_t virtualMemoryTotal;
        size_t virtualMemoryUsed;
        size_t programMemoryUsedInclusive;
        size_t programMemoryUsedExclusive;
    };

    enum class PlatformWindowEvent : uint8_t
    {
        FullScreenRequest,
        FullScreenExit,
        Focus,
        Unfocus,
        Resize,
        Visible,
        Invisible,
        Close
    };

    struct PlatformWindowDescriptor
    {
        const char* title;
        int2 position = PK_INT2_MINUS_ONE;
        int2 size = PK_INT2_ONE;
        int2 sizemin = PK_INT2_MINUS_ONE;
        int2 sizemax = PK_INT2_MINUS_ONE;
        bool isVisible = false;
        bool isResizable = false;
        bool isFloating = false;
        bool useDpiScaling = false;
        bool autoActivate = false;
        bool useEmbeddedIcon = false;
    };

    struct IPlatformWindowListener
    {
        virtual ~IPlatformWindowListener() = 0;
        virtual bool IPlatformWindow_OnEvent(PlatformWindow* window, PlatformWindowEvent evt) = 0;
    };

    // OS Window interface for the native environment.
    struct PlatformWindow : public NoCopy, public InputDevice
    {
        virtual ~PlatformWindow() = 0;

        virtual int4 GetRect() const = 0;
        virtual int2 GetMonitorResolution() const = 0;
        virtual float2 GetCursorPosition() const = 0;
        virtual bool IsMinimized() const = 0;
        virtual bool IsMaximized() const = 0;
        virtual bool IsClosing() const = 0;
        virtual bool IsFocused() const = 0;
        virtual void* GetNativeWindowHandle() const = 0;
        virtual void* GetNativeMonitorHandle() const = 0;

        virtual void SetRect(const int4& rect) = 0;
        virtual void SetCursorPosition(const float2& position) = 0;
        virtual void SetCursorLock(bool lock, bool visible) = 0;
        virtual void SetIcon(unsigned char* pixels, const int2& resolution) = 0;

        virtual void SetVisible(bool value) = 0;
        virtual void SetFullScreen(bool value) = 0;
        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void Restore() = 0;
        virtual void Focus() = 0;
        virtual void Close() = 0;

        virtual void SetListener(IPlatformWindowListener* listener) = 0;

        inline InputType GetInputType() final { return InputType::KeyboardAndMouse; }

        constexpr int2 GetSize() const { return GetRect().zw; }
        constexpr int2 GetPosition() const { return GetRect().xy; }
        inline void SetSize(const int2& size) { SetRect({ GetPosition(), size }); }
        inline void SetPosition(const int2& position) { SetRect({ position, GetSize() }); }
        inline void SetCursorPosToCenter() { SetCursorPosition(GetSize() / 2); }
    };
}
