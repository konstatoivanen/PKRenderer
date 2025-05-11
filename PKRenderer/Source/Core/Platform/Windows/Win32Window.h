#pragma once
#include "Core/Platform/PlatformInterfaces.h"

#if PK_PLATFORM_WINDOWS

#include <bitset>
#include "Core/Utilities/FastSet.h"

namespace PK
{
    struct Win32Window : public PlatformWindow
    {
        friend struct Win32WindowManager;

        constexpr static const LPCWSTR CLASS_MAIN = L"PK_PLATFORM_WIN32_WINDOW_CLASS_MAIN";
        constexpr static const LPCWSTR CLASS_HELPER = L"PK_PLATFORM_WIN32_WINDOW_CLASS_HELPER";
        constexpr static const LPCWSTR WINDOW_PROP = L"PK_PLATFORM_WIN32_WINDOW_PROP";

        Win32Window(const PlatformWindowDescriptor& descriptor);
        ~Win32Window();

        int4 GetRect() const final;
        int2 GetMonitorResolution() const final;
        int2 GetCursorPosition() const final;
        inline bool GetIsClosing() const final { return state.isClosing; }
        inline bool GetIsFocused() const final { return state.isFocused; }
        inline void* GetNativeMonitorHandle() const { return ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST); }

        void SetRect(const int4& rect) final;
        void SetCursorPosition(const int2& position) final;
        void SetCursorLock(bool lock, bool hide) final;
        void SetRawMouseInput(bool value) final;
        void SetIcon(unsigned char* pixels, const int2& resolution) final;

        void SetVisible(bool value) final;
        void Minimize() final;
        void Maximize() final;
        void SetBorderless(bool value, bool maximize = false) final;
        void SetFullScreen(bool value) final;
        void Restore() final;

        inline void RegisterWindowListener(IPlatformWindowListener* listener) final { m_windowListeners.Add(listener); }
        inline void UnregisterWindowListener(IPlatformWindowListener* listener) final { m_windowListeners.Remove(listener); }
        inline void RegisterInputListener(IPlatformWindowInputListener* listener) final { m_inputListeners.Add(listener); }
        inline void UnregisterInputListener(IPlatformWindowInputListener* listener) final { m_inputListeners.Remove(listener); }

        protected:
            void OnPollEvents();
            void OnWaitEvents();

            void ValidateResolution();
            void BeginLockingCursor();
            void EndLockingCursor();
            void UpdateCursor();
            void CursorMovedEvent(const int2& position);
            void UpdateRawMouseInput(LPARAM lParam);

            void DispatchWindowEvent(PlatformWindowEvent evt);
            void DispatchKeyEvent(InputKey key, bool isDown);
            void DispatchMouseMoveEvent(const int2& position);
            void DispatchScrollEvent(const float2& offset);
            void DispatchCharacterEvent(uint32_t character);
            void DispatchDrop(const char* const* paths, uint32_t count);
            void OnFocusChanged(bool value);
            void OnClose();
            bool IsAnyMouseKeyDown() const;

            LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

            HWND handle = NULL;
            HICON icon = NULL;

            struct
            {
                bool isVisible = false;
                bool isMinimized = false;
                bool isMaximized = false;
                bool isFullScreen = false;
                bool isFocused = false;
                bool isClosing = false;
                bool isUsingRawMotion = false;
                bool isCursorTracked = false;
                bool isPendingActivate = false;
            } 
            state;

            struct
            {
                bool inMaximize = false;
                bool inFullScreen = false;
                bool inResize = false;
                bool inFrameAction = false;
            }
            scope;

            struct
            {
                bool cursorHide = false;
                bool cursorLock = false;
                bool useRawMouseInput = false;
                bool isBorderless = false;
                bool isFullScreen = false;
                bool useDpiScaling = false;
            }
            options;

            int2 size_min = PK_INT2_ZERO;
            int2 size_max = PK_INT2_ZERO;
            int2 cached_clientsize = PK_INT2_ZERO;
            int2 cached_cursorpos = PK_INT2_ZERO;
            int2 virt_cursorpos = PK_INT2_ZERO;

            std::bitset<(int)InputKey::Count> cached_keystates;
            WCHAR cached_lastHighSurrogate = 0;

            PointerSet<IPlatformWindowListener> m_windowListeners;
            PointerSet<IPlatformWindowInputListener> m_inputListeners;
    };
}

#endif