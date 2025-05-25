#pragma once
#include "Core/Platform/PlatformInterfaces.h"

#if PK_PLATFORM_WINDOWS

#include <bitset>

namespace PK
{
    struct Win32Window : public PlatformWindow
    {
        constexpr static const LPCWSTR CLASS_MAIN = L"PK_PLATFORM_WIN32_WINDOW_CLASS_MAIN";
        constexpr static const LPCWSTR CLASS_HELPER = L"PK_PLATFORM_WIN32_WINDOW_CLASS_HELPER";
        constexpr static const LPCWSTR WINDOW_PROP = L"PK_PLATFORM_WIN32_WINDOW_PROP";

        Win32Window(const PlatformWindowDescriptor& descriptor);
        ~Win32Window();

        int4 GetRect() const final;
        int2 GetMonitorResolution() const final;
        float2 GetCursorPosition() const final;
        inline bool IsMinimized() const { return m_isMinimized; }
        inline bool IsMaximized() const { return m_isMaximized; }
        inline bool IsClosing() const final { return m_isClosing; }
        inline bool IsFocused() const final { return m_isFocused; }
        inline void* GetNativeWindowHandle() const { return m_handle; }
        inline void* GetNativeMonitorHandle() const { return ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST); }

        float2 GetInputCursorPosition() final { return m_cursorposVirtual; }
        const InputKeyState& GetInputKeyState() final { return m_keyState; }
        float GetInputAnalogAxis(InputKey neg, InputKey pos) final;
        void SetUseRawInput(bool value) final;

        void SetRect(const int4& rect) final;
        void SetCursorPosition(const float2& position) final;
        void SetCursorLock(bool lock, bool visible) final;
        void SetIcon(unsigned char* pixels, const int2& resolution) final;

        void SetVisible(bool value) final;
        void SetFullScreen(bool value) final;
        void Minimize() final;
        void Maximize() final;
        void Restore() final;
        void Focus() final;
        void Close() final;

        inline Win32Window*& GetNext() { return m_nextWindow; }

        inline void SetListener(IPlatformWindowListener* listener) final { m_windowListener = listener; }

        void OnPollEvents();

        LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

        private:
            void ValidateResolution();
            void UpdateCursor();
            void UpdateRawInput(LPARAM lParam);

            bool DispatchWindowOnEvent(PlatformWindowEvent evt);
            void DispatchInputOnKey(InputKey key, bool isDown);
            void DispatchInputOnMouseMoved(const float2& position);
            void DispatchInputOnScroll(uint32_t axis, float offset);
            void DispatchInputOnCharacter(uint32_t character);
            void DispatchInputOnDrop(WPARAM wParam);
            void OnFocusChanged(bool value);
            bool IsAnyMouseKeyDown() const;

            HWND m_handle = NULL;
            HICON m_icon = NULL;

            bool m_isVisible = false;
            bool m_isMinimized = false;
            bool m_isMaximized = false;
            bool m_isFocused = false;
            bool m_isClosing = false;
            bool m_isUsingRawInput = false;
            bool m_isPendingActivate = false;
            bool m_isFullScreen = false;

            bool m_isInFrameAction = false;
            bool m_isResizing = false;
            bool m_isAcquiringFullScreen = false;
            bool m_isMaximizing = false;

            bool m_cursorHide = false;
            bool m_cursorLock = false;
            bool m_useRawInput = false;
            bool m_useDpiScaling = false;
            bool m_isResizable = false;
            int2 m_sizeMin = PK_INT2_ZERO;
            int2 m_sizeMax = PK_INT2_ZERO;

            int2 m_clientsize = PK_INT2_ZERO;
            int4 m_restoreRect = PK_INT4_ZERO;
            float2 m_cursorpos = PK_FLOAT2_ZERO;
            float2 m_cursorposVirtual = PK_FLOAT2_ZERO;
            float2 m_scroll[2]{};

            InputKeyState m_keyState;
            WCHAR m_lastHighSurrogate = 0;

            IPlatformWindowListener* m_windowListener = nullptr;
            Win32Window* m_nextWindow = nullptr;
    };
}

#endif
