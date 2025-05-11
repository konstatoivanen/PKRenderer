#include "PrecompiledHeader.h"

#if PK_PLATFORM_WINDOWS

#include "Win32Driver.h"
#include "Win32WindowManager.h"

namespace PK
{
    Win32WindowManager::Win32WindowManager(HINSTANCE instance)
    {
        // Helper window class with helper window proc (always active)
        {
            WNDCLASSEXW wc = { sizeof(wc) };
            wc.style = CS_OWNDC;
            wc.lpfnWndProc = WindowProc_Helper;
            wc.hInstance = instance;
            wc.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
            wc.lpszClassName = Win32Window::CLASS_HELPER;

            if (!(classHelper = ::RegisterClassExW(&wc)))
            {
                throw std::runtime_error("Failed to create window helper class.");
            }
        }

        // Main window class for actual display windows.
        {
            WNDCLASSEXW wc = { sizeof(wc) };
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WindowProc_Main;
            wc.hInstance = instance;
            wc.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
            wc.lpszClassName = Win32Window::CLASS_MAIN;
            wc.hIcon = (HICON)::LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

            if (!(classMain = ::RegisterClassExW(&wc)))
            {
                throw std::runtime_error("Failed to create window main class.");
            }
        }

        helper = ::CreateWindowExW
        (
            WS_EX_OVERLAPPEDWINDOW,
            Win32Window::CLASS_HELPER,
            L"PK_PLATFORM_WIN32_WINDOW_HELPER",
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 1, 1,
            NULL, NULL,
            instance,
            NULL
        );

        if (!helper)
        {
            throw std::runtime_error("Failed to create helper window.");
        }

        ::ShowWindow(helper, SW_HIDE);

        {
            memset(keycode_to_native, -1, sizeof(keycode_to_native));
            memset(native_to_keycode, 0, sizeof(native_to_keycode));

            native_to_keycode[0x00B] = InputKey::Alpha0;
            native_to_keycode[0x002] = InputKey::Alpha1;
            native_to_keycode[0x003] = InputKey::Alpha2;
            native_to_keycode[0x004] = InputKey::Alpha3;
            native_to_keycode[0x005] = InputKey::Alpha4;
            native_to_keycode[0x006] = InputKey::Alpha5;
            native_to_keycode[0x007] = InputKey::Alpha6;
            native_to_keycode[0x008] = InputKey::Alpha7;
            native_to_keycode[0x009] = InputKey::Alpha8;
            native_to_keycode[0x00A] = InputKey::Alpha9;
            native_to_keycode[0x01E] = InputKey::A;
            native_to_keycode[0x030] = InputKey::B;
            native_to_keycode[0x02E] = InputKey::C;
            native_to_keycode[0x020] = InputKey::D;
            native_to_keycode[0x012] = InputKey::E;
            native_to_keycode[0x021] = InputKey::F;
            native_to_keycode[0x022] = InputKey::G;
            native_to_keycode[0x023] = InputKey::H;
            native_to_keycode[0x017] = InputKey::I;
            native_to_keycode[0x024] = InputKey::J;
            native_to_keycode[0x025] = InputKey::K;
            native_to_keycode[0x026] = InputKey::L;
            native_to_keycode[0x032] = InputKey::M;
            native_to_keycode[0x031] = InputKey::N;
            native_to_keycode[0x018] = InputKey::O;
            native_to_keycode[0x019] = InputKey::P;
            native_to_keycode[0x010] = InputKey::Q;
            native_to_keycode[0x013] = InputKey::R;
            native_to_keycode[0x01F] = InputKey::S;
            native_to_keycode[0x014] = InputKey::T;
            native_to_keycode[0x016] = InputKey::U;
            native_to_keycode[0x02F] = InputKey::V;
            native_to_keycode[0x011] = InputKey::W;
            native_to_keycode[0x02D] = InputKey::X;
            native_to_keycode[0x015] = InputKey::Y;
            native_to_keycode[0x02C] = InputKey::Z;

            native_to_keycode[0x028] = InputKey::Apostrophe;
            native_to_keycode[0x02B] = InputKey::BackSlash;
            native_to_keycode[0x033] = InputKey::Comma;
            native_to_keycode[0x00D] = InputKey::Equal;
            native_to_keycode[0x029] = InputKey::GraveAccent;
            native_to_keycode[0x01A] = InputKey::LeftBracket;
            native_to_keycode[0x00C] = InputKey::Minus;
            native_to_keycode[0x034] = InputKey::Period;
            native_to_keycode[0x01B] = InputKey::RightBracket;
            native_to_keycode[0x027] = InputKey::Semicolon;
            native_to_keycode[0x035] = InputKey::Slash;
            native_to_keycode[0x056] = InputKey::World2;

            native_to_keycode[0x00E] = InputKey::Backspace;
            native_to_keycode[0x153] = InputKey::Del;
            native_to_keycode[0x14F] = InputKey::End;
            native_to_keycode[0x01C] = InputKey::Enter;
            native_to_keycode[0x001] = InputKey::Escape;
            native_to_keycode[0x147] = InputKey::Home;
            native_to_keycode[0x152] = InputKey::Insert;
            native_to_keycode[0x15D] = InputKey::Menu;
            native_to_keycode[0x151] = InputKey::PageDown;
            native_to_keycode[0x149] = InputKey::PageUp;
            native_to_keycode[0x045] = InputKey::Pause;
            native_to_keycode[0x039] = InputKey::Space;
            native_to_keycode[0x00F] = InputKey::Tab;
            native_to_keycode[0x03A] = InputKey::CapsLock;
            native_to_keycode[0x145] = InputKey::NumLock;
            native_to_keycode[0x046] = InputKey::ScrollLock;
            native_to_keycode[0x03B] = InputKey::F1;
            native_to_keycode[0x03C] = InputKey::F2;
            native_to_keycode[0x03D] = InputKey::F3;
            native_to_keycode[0x03E] = InputKey::F4;
            native_to_keycode[0x03F] = InputKey::F5;
            native_to_keycode[0x040] = InputKey::F6;
            native_to_keycode[0x041] = InputKey::F7;
            native_to_keycode[0x042] = InputKey::F8;
            native_to_keycode[0x043] = InputKey::F9;
            native_to_keycode[0x044] = InputKey::F10;
            native_to_keycode[0x057] = InputKey::F11;
            native_to_keycode[0x058] = InputKey::F12;
            native_to_keycode[0x064] = InputKey::F13;
            native_to_keycode[0x065] = InputKey::F14;
            native_to_keycode[0x066] = InputKey::F15;
            native_to_keycode[0x067] = InputKey::F16;
            native_to_keycode[0x068] = InputKey::F17;
            native_to_keycode[0x069] = InputKey::F18;
            native_to_keycode[0x06A] = InputKey::F19;
            native_to_keycode[0x06B] = InputKey::F20;
            native_to_keycode[0x06C] = InputKey::F21;
            native_to_keycode[0x06D] = InputKey::F22;
            native_to_keycode[0x06E] = InputKey::F23;
            native_to_keycode[0x076] = InputKey::F24;
            native_to_keycode[0x038] = InputKey::LeftAlt;
            native_to_keycode[0x01D] = InputKey::LeftControl;
            native_to_keycode[0x02A] = InputKey::LeftShift;
            native_to_keycode[0x15B] = InputKey::LeftSuper;
            native_to_keycode[0x137] = InputKey::PrintScreen;
            native_to_keycode[0x138] = InputKey::RightAlt;
            native_to_keycode[0x11D] = InputKey::RightControl;
            native_to_keycode[0x036] = InputKey::RightShift;
            native_to_keycode[0x15C] = InputKey::RightSuper;
            native_to_keycode[0x150] = InputKey::Down;
            native_to_keycode[0x14B] = InputKey::Left;
            native_to_keycode[0x14D] = InputKey::Right;
            native_to_keycode[0x148] = InputKey::Up;

            native_to_keycode[0x052] = InputKey::KeyPad0;
            native_to_keycode[0x04F] = InputKey::KeyPad1;
            native_to_keycode[0x050] = InputKey::KeyPad2;
            native_to_keycode[0x051] = InputKey::KeyPad3;
            native_to_keycode[0x04B] = InputKey::KeyPad4;
            native_to_keycode[0x04C] = InputKey::KeyPad5;
            native_to_keycode[0x04D] = InputKey::KeyPad6;
            native_to_keycode[0x047] = InputKey::KeyPad7;
            native_to_keycode[0x048] = InputKey::KeyPad8;
            native_to_keycode[0x049] = InputKey::KeyPad9;
            native_to_keycode[0x04E] = InputKey::KeyPadAdd;
            native_to_keycode[0x053] = InputKey::KeyPadDecimal;
            native_to_keycode[0x135] = InputKey::KeyPadDivide;
            native_to_keycode[0x11C] = InputKey::KeyPadEnter;
            native_to_keycode[0x059] = InputKey::KeyPadEqual;
            native_to_keycode[0x037] = InputKey::KeyPadMultiply;
            native_to_keycode[0x04A] = InputKey::KeyPadSubtract;

            for (auto i = 0u; i < 512u; ++i)
            {
                if (native_to_keycode[i] != InputKey::None)
                {
                    keycode_to_native[(uint32_t)native_to_keycode[i]] = i;
                }
            }
        }
    }

    Win32WindowManager::~Win32WindowManager()
    {
        for (auto i = 0u; i < windows.GetCount(); ++i)
        {
            windows.GetValues()[i] = nullptr;
        }

        ::ClipCursor(NULL);

        if (helper)
        {
            ::DestroyWindow((HWND)helper);
        }

        if (classHelper)
        {
            ::UnregisterClassW(Win32Window::CLASS_HELPER, nullptr);
            classHelper = NULL;
        }

        if (classMain)
        {
            ::UnregisterClassW(Win32Window::CLASS_MAIN, nullptr);
            classMain = NULL;
        }

        if (rawInput)
        {
            free(rawInput);
        }
    }

    
    Win32WindowManager* Win32WindowManager::Get()
    {
        return PlatformDriver::GetNative<Win32Driver>()->windowManager.get();
    }


    PlatformWindow* Win32WindowManager::CreateWindow(const PlatformWindowDescriptor& descriptor)
    {
        auto nativeWindow = new Win32Window(descriptor);
        auto index = 0u;
        windows.AddKey(nativeWindow, &index);
        windows.GetValues()[index] = Unique<Win32Window>(nativeWindow);
        return nativeWindow;
    }

    void Win32WindowManager::DestroyWindow(PlatformWindow* window)
    {
        windows.Remove(window);
    }


    void Win32WindowManager::PollEvents()
    {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                for (auto i = 0u; i < windows.GetCount(); ++i)
                {
                    windows.GetValueAt(i)->OnClose();
                }
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        HWND activeHandle = ::GetActiveWindow();

        if (activeHandle)
        {
            auto activeWindow = (Win32Window*)GetPropW(activeHandle, Win32Window::WINDOW_PROP);

            if (activeWindow)
            {
                activeWindow->OnPollEvents();
            }
        }

        if (disabledCursorWindow)
        {
            auto center = disabledCursorWindow->GetResolution() / 2;

            if (disabledCursorWindow->cached_cursorpos != center)
            {
                disabledCursorWindow->SetCursorPosition(center);
            }
        }
    }

    void Win32WindowManager::WaitEvents()
    {
        ::WaitMessage();
        PollEvents();
    }


    LRESULT Win32WindowManager::WindowProc_Helper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_INPUTLANGCHANGE:
            {
                //@TODO update input language _glfwUpdateKeyNamesWin32();
                break;
            }

            case WM_DISPLAYCHANGE:
            {
                // @TODO update monitors
                break;
            }

            case WM_DEVICECHANGE:
            {
                // @TODO handle devices
                break;
            }
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    LRESULT Win32WindowManager::WindowProc_Main(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Win32Window* window = (Win32Window*)::GetPropW(hWnd, Win32Window::WINDOW_PROP);

        if (window)
        {
            return window->WindowProc(uMsg, wParam, lParam);
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

#endif
