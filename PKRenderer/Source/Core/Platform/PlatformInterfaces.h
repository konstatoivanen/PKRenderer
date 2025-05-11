#pragma once
#include <exception>
#include "Core/Platform/Platform.h"
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NativeInterface.h"
#include "Core/Input/InputKey.h"
#include "Core/Math/Math.h"

namespace PK
{
    enum class PlatformWindowEvent : uint8_t
    {
        Error,
        CursorEnter,
        CursorExit,
        FullScreenRequest,
        FullScreenExit,
        Focus,
        Unfocus,
        Move,
        Resize,
        Visible,
        Invisible,
        Close
    };

    struct PlatformWindowDescriptor
    {
        std::string title;
        int2 position = PK_INT2_MINUS_ONE;
        int2 resolution = PK_INT2_ONE;
        int2 resolutionMin = PK_INT2_MINUS_ONE;
        int2 resolutionMax = PK_INT2_MINUS_ONE;
        bool isVisible = false;
        bool isResizable = false;
        bool isFloating = false;
        bool useDpiScaling = false;
        bool scaleFrameBuffer = false;
        bool activateOnFirstShow = false;
    };

    struct IPlatformWindowListener
    {
        virtual ~IPlatformWindowListener() = 0;
        virtual bool IPlatformWindow_OnEvent(PlatformWindow* window, PlatformWindowEvent evt) = 0;
    };

    struct IPlatformWindowInputListener
    {
        virtual ~IPlatformWindowInputListener() = 0;
        virtual bool IPlatformWindowInput_OnKey(PlatformWindow* window, InputKey key, bool isDown) = 0;
        virtual bool IPlatformWindowInput_OnMouseMove(PlatformWindow* window, const int2& position) = 0;
        virtual bool IPlatformWindowInput_OnScroll(PlatformWindow* window, const float2& offset) = 0;
        virtual bool IPlatformWindowInput_OnCharacter(PlatformWindow* window, uint32_t character) = 0;
        virtual bool IPlatformWindowInput_OnDrop(PlatformWindow* window, const char* const* paths, uint32_t count) = 0;
    };

    // OS Window interface for the native environment.
    struct PlatformWindow : public NoCopy, public NativeInterface<PlatformWindow>
    {
        virtual ~PlatformWindow() = 0;

        virtual int4 GetRect() const = 0;
        virtual int2 GetMonitorResolution() const = 0;
        virtual int2 GetCursorPosition() const = 0;
        virtual bool GetIsClosing() const = 0;
        virtual bool GetIsFocused() const = 0;
        virtual void* GetNativeMonitorHandle() const = 0;

        virtual void SetRect(const int4& rect) = 0;
        virtual void SetCursorPosition(const int2& position) = 0;
        virtual void SetCursorLock(bool lock, bool hide) = 0;
        virtual void SetRawMouseInput(bool value) = 0;
        virtual void SetIcon(unsigned char* pixels, const int2& resolution) = 0;

        virtual void SetVisible(bool value) = 0;
        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void SetBorderless(bool value, bool maximize = false) = 0;
        virtual void SetFullScreen(bool value) = 0;
        virtual void Restore() = 0;

        virtual void RegisterWindowListener(IPlatformWindowListener* listener) = 0;
        virtual void UnregisterWindowListener(IPlatformWindowListener* listener) = 0;
        virtual void RegisterInputListener(IPlatformWindowInputListener* listener) = 0;
        virtual void UnregisterInputListener(IPlatformWindowInputListener* listener) = 0;

        constexpr int2 GetResolution() const { return GetRect().zw; }
        constexpr int2 GetPosition() const { return GetRect().xy; }
        inline void SetResolution(const int2& resolution) { SetRect({ GetPosition(), resolution }); }
        inline void SetPosition(const int2& position) { SetRect({ position, GetResolution() }); }
        inline void SetCursorPosToCenter() { SetCursorPosition(GetResolution() / 2); }
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
        virtual void* GetMonitorHandle(const int2& point, bool preferPrimary) const = 0;
        virtual int4 GetMonitorRect(const int2& point, bool preferPrimary) const = 0;

        virtual PlatformWindow* CreateWindow(const PlatformWindowDescriptor& descriptor) = 0;
        virtual void DestroyWindow(PlatformWindow* window) = 0;

        virtual void SetConsoleColor(uint32_t color) const = 0;
        virtual void SetConsoleVisible(bool value) const = 0;
        virtual bool RemoteProcess(const char* executable, const char* arguments, std::string& outError) const = 0;

        template<typename TChild>
        inline static TChild* GetNative() { return Get()->GetNative<TChild>(); }

        static inline PlatformDriver* Get() { return s_instance; }
        protected: inline static PlatformDriver* s_instance = nullptr;
    };
}
