#pragma once
#include <exception>
#include "Core/Platform/Platform.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NativeInterface.h"
#include "Core/Input/InputDevice.h"
#include "Core/Math/Math.h"

namespace PK
{
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
        bool activateOnFirstShow = false;
    };

    struct IPlatformWindowListener
    {
        virtual ~IPlatformWindowListener() = 0;
        virtual bool IPlatformWindow_OnEvent(PlatformWindow* window, PlatformWindowEvent evt) = 0;
    };

    // OS Window interface for the native environment.
    struct PlatformWindow : public NoCopy, public NativeInterface<PlatformWindow>, public InputDevice
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

        virtual void SetListener(IPlatformWindowListener* listener) = 0;

        inline InputType GetInputType() final { return InputType::KeyboardAndMouse; }

        constexpr int2 GetSize() const { return GetRect().zw; }
        constexpr int2 GetPosition() const { return GetRect().xy; }
        inline void SetSize(const int2& size) { SetRect({ GetPosition(), size }); }
        inline void SetPosition(const int2& position) { SetRect({ position, GetSize() }); }
        inline void SetCursorPosToCenter() { SetCursorPosition(GetSize() / 2); }
    };

    struct PlatformDriver : public NoCopy, public NativeInterface<PlatformDriver>
    {
        PlatformDriver() { if (s_instance != nullptr) throw std::exception("Trying initialize multiple native interfaces!"); s_instance = this; }
        virtual ~PlatformDriver() = 0;

        virtual void PollEvents() = 0;
        virtual void WaitEvents() = 0;

        virtual void* GetProcess() const = 0;
        virtual void* GetHelperWindow() const = 0;
        virtual void* GetProcAddress(void* handle, const char* name) const = 0;

        virtual void* LoadLibrary(const char* path) const = 0;
        virtual void FreeLibrary(void* handle) const = 0;

        virtual bool GetHasFocus() const = 0;
        virtual int2 GetDesktopSize() const = 0;
        virtual int4 GetMonitorRect(const int2& point, bool preferPrimary) const = 0;
        virtual void* GetNativeMonitorHandle(const int2& point, bool preferPrimary) const = 0;

        virtual PlatformWindow* CreateWindow(const PlatformWindowDescriptor& descriptor) = 0;
        virtual void DestroyWindow(PlatformWindow* window) = 0;

        virtual void SetInputHandler(InputHandler* handler) = 0;

        virtual std::string GetClipboardString() = 0;
        virtual void SetClipboardString(const char* str) = 0;

        virtual void SetConsoleColor(uint32_t color) const = 0;
        virtual void SetConsoleVisible(bool value) const = 0;
        virtual bool RemoteProcess(const char* executable, const char* arguments, std::string& outError) const = 0;

        static inline PlatformDriver* Get() { return s_instance; }
        protected: inline static PlatformDriver* s_instance = nullptr;
    };
}
