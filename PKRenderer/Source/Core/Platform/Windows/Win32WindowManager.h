#pragma once
#include "Core/Platform/PlatformInterfaces.h"

#if PK_PLATFORM_WINDOWS

#include "Core/Utilities/FastMap.h"
#include "Core/Platform/Windows/Win32Window.h"

namespace PK
{
    struct Win32WindowManager : public NoCopy
    {
        friend struct Win32Window;

        Win32WindowManager(HINSTANCE instance);
        ~Win32WindowManager();

        static Win32WindowManager* Get();

        PlatformWindow* CreateWindow(const PlatformWindowDescriptor& descriptor);
        void DestroyWindow(PlatformWindow* window);
        
        void PollEvents();
        void WaitEvents();

        constexpr HWND GetHelperWindow() const { return helper; }

        protected:
            static LRESULT CALLBACK WindowProc_Helper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
            static LRESULT CALLBACK WindowProc_Main(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

            ATOM classMain;
            ATOM classHelper;
            HWND helper;

            int2 restore_cursorpos;
            Win32Window* disabledCursorWindow;
            Win32Window* lockedCursorWindow;
            RAWINPUT* rawInput = nullptr;
            uint32_t rawInputSize = 0u;

            InputKey native_to_keycode[512];
            int16_t keycode_to_native[(int32_t)InputKey::Count];

            FastMap<PlatformWindow*, Unique<Win32Window>> windows;
    };
}

#endif