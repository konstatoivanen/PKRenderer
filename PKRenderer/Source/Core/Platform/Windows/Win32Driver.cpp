#include "PrecompiledHeader.h"

#if PK_PLATFORM_WINDOWS
#include "Win32Driver.h"

PFN_DirectInput8Create pkfn_DirectInput8Create = nullptr;
PFN_XInputGetCapabilities pkfn_XInputGetCapabilities = nullptr;
PFN_XInputGetState pkfn_XInputGetState = nullptr;
PFN_SetProcessDPIAware pkfn_SetProcessDPIAware = nullptr;
PFN_EnableNonClientDpiScaling pkfn_EnableNonClientDpiScaling = nullptr;
PFN_SetProcessDpiAwarenessContext pkfn_SetProcessDpiAwarenessContext = nullptr;
PFN_SetProcessDpiAwareness pkfn_SetProcessDpiAwareness = nullptr;
PFN_GetDpiForWindow pkfn_GetDpiForWindow = nullptr;
PFN_GetDpiForMonitor pkfn_GetDpiForMonitor = nullptr;
PFN_ChangeWindowMessageFilterEx pkfn_ChangeWindowMessageFilterEx = nullptr;
PFN_AdjustWindowRectExForDpi pkfn_AdjustWindowRectExForDpi = nullptr;
PFN_DwmSetWindowAttribute pkfn_DwmSetWindowAttribute = nullptr;
PFN_RtlVerifyVersionInfo pkfn_RtlVerifyVersionInfo = nullptr;

namespace PK
{
    static int s_localPtr;

    // These didnt want to get included from hidclass.h for some reason
    static const GUID s_GUID_DEVINTERFACE_HID = { 0x4d1e55b2,0xf16f,0x11cf,{0x88,0xcb,0x00,0x11,0x11,0x00,0x00,0x30} };
    static const GUID s_IID_IDirectInput8W = { 0xbf798031,0x483a,0x4da2,{0xaa,0x99,0x5d,0x64,0xed,0x36,0x97,0x00} };

    Win32Driver::Win32Driver()
    {
        if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (const WCHAR*)&s_localPtr,
            (HMODULE*)&instance))
        {
            throw std::runtime_error("Failed to get program HINSTANCE");
        }

        user32_handle = LoadLibrary("user32.dll");
        dinput8_handle = LoadLibrary("dinput8.dll");
        shcore_handle = LoadLibrary("shcore.dll");
        ntdll_handle = LoadLibrary("ntdll.dll");
        dwmapi_handle = LoadLibrary("dwmapi.dll");

        if (!user32_handle)
        {
            throw std::runtime_error("Failed to load user32.dll");
        }

        pkfn_SetProcessDPIAware = (PFN_SetProcessDPIAware)GetProcAddress(user32_handle, "SetProcessDPIAware");
        pkfn_ChangeWindowMessageFilterEx = (PFN_ChangeWindowMessageFilterEx)GetProcAddress(user32_handle, "ChangeWindowMessageFilterEx");
        pkfn_EnableNonClientDpiScaling = (PFN_EnableNonClientDpiScaling)GetProcAddress(user32_handle, "EnableNonClientDpiScaling");
        pkfn_SetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(user32_handle, "SetProcessDpiAwarenessContext");
        pkfn_GetDpiForWindow = (PFN_GetDpiForWindow)GetProcAddress(user32_handle, "GetDpiForWindow");
        pkfn_AdjustWindowRectExForDpi = (PFN_AdjustWindowRectExForDpi)GetProcAddress(user32_handle, "AdjustWindowRectExForDpi");

        if (dinput8_handle)
        {
            pkfn_DirectInput8Create = (PFN_DirectInput8Create)GetProcAddress(dinput8_handle, "DirectInput8Create");

            if (FAILED(PK_DirectInput8Create(instance, DIRECTINPUT_VERSION, s_IID_IDirectInput8W, (void**)&dinput8_api, NULL)))
            {
                dinput8_api = nullptr;
            }
        }

        if (shcore_handle)
        {
            pkfn_SetProcessDpiAwareness = (PFN_SetProcessDpiAwareness)GetProcAddress(shcore_handle, "SetProcessDpiAwareness");
            pkfn_GetDpiForMonitor = (PFN_GetDpiForMonitor)GetProcAddress(shcore_handle, "GetDpiForMonitor");
        }

        if (ntdll_handle)
        {
            pkfn_RtlVerifyVersionInfo = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdll_handle, "RtlVerifyVersionInfo");
        }

        if (dwmapi_handle)
        {
            pkfn_DwmSetWindowAttribute = (PFN_DwmSetWindowAttribute)GetProcAddress(dwmapi_handle, "DwmSetWindowAttribute");
        }

        const char* xinputModuleNames[] =
        {
            "xinput1_4.dll",
            "xinput1_3.dll",
            "xinput9_1_0.dll",
            "xinput1_2.dll",
            "xinput1_1.dll"
        };

        for (auto i = 0u; i < 5u; ++i)
        {
            xinput_handle = LoadLibrary(xinputModuleNames[i]);

            if (xinput_handle)
            {
                pkfn_XInputGetCapabilities = (PFN_XInputGetCapabilities)GetProcAddress(xinput_handle, "XInputGetCapabilities");
                pkfn_XInputGetState = (PFN_XInputGetState)GetProcAddress(xinput_handle, "XInputGetState");
                break;
            }
        }

        if (PK_PLATFORM_WINDOWS_IS_10_1703_OR_GREATER())
        {
            PK_SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
        else if (PK_PLATFORM_WINDOWS_IS_8_1_OR_GREATER())
        {
            PK_SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        }
        else
        {
            PK_SetProcessDPIAware();
        }

        // Helper window class with helper window proc (always active)
        {
            WNDCLASSEXW wc = { sizeof(wc) };
            wc.style = CS_OWNDC;
            wc.lpfnWndProc = WindowProc_Helper;
            wc.hInstance = instance;
            wc.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
            wc.lpszClassName = Win32Window::CLASS_HELPER;

            if (!(m_windowClassHelper = ::RegisterClassExW(&wc)))
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

            if (!(m_windowClassMain = ::RegisterClassExW(&wc)))
            {
                throw std::runtime_error("Failed to create window main class.");
            }
        }

        m_windowInstanceHelper = ::CreateWindowExW
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

        if (!m_windowInstanceHelper)
        {
            throw std::runtime_error("Failed to create helper window.");
        }

        ::ShowWindow(m_windowInstanceHelper, SW_HIDE);

        #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
        {
            DEV_BROADCAST_DEVICEINTERFACE_W dbi;
            ZeroMemory(&dbi, sizeof(dbi));
            dbi.dbcc_size = sizeof(dbi);
            dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            dbi.dbcc_classguid = s_GUID_DEVINTERFACE_HID;
            m_deviceNotificationHandle = ::RegisterDeviceNotificationW(m_windowInstanceHelper, (DEV_BROADCAST_HDR*)&dbi, DEVICE_NOTIFY_WINDOW_HANDLE);
        }
        #endif

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

    Win32Driver::~Win32Driver()
    {
        auto head = m_windowHead;

        while (head)
        {
            m_windowHead = head->GetNext();

            if (m_inputHandler)
            {
                m_inputHandler->InputHandler_OnDisconnect(head);
            }

            delete static_cast<Win32Window*>(head);
            head = m_windowHead;
        }

        ::ClipCursor(NULL);

        if (m_deviceNotificationHandle)
        {
            ::UnregisterDeviceNotification(m_deviceNotificationHandle);
        }

        if (m_windowInstanceHelper)
        {
            ::DestroyWindow((HWND)m_windowInstanceHelper);
        }

        if (m_windowClassHelper)
        {
            ::UnregisterClassW(Win32Window::CLASS_HELPER, nullptr);
            m_windowClassHelper = NULL;
        }

        if (m_windowClassMain)
        {
            ::UnregisterClassW(Win32Window::CLASS_MAIN, nullptr);
            m_windowClassMain = NULL;
        }

        if (m_rawInput)
        {
            free(m_rawInput);
        }

        if (dinput8_api)
        {
            IDirectInput8_Release(dinput8_api);
            dinput8_api = nullptr;
        }

        FreeLibrary(user32_handle);
        FreeLibrary(dinput8_handle);
        FreeLibrary(shcore_handle);
        FreeLibrary(ntdll_handle);
        FreeLibrary(dwmapi_handle);
        FreeLibrary(xinput_handle);
    }

    
    void Win32Driver::PollEvents()
    {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                auto head = m_windowHead;

                while (head)
                {
                    head->OnClose();
                    head = head->GetNext();
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

        if (m_disabledCursorWindow)
        {
            m_disabledCursorWindow->SetCursorPosToCenter();
        }
    }

    void Win32Driver::WaitEvents()
    {
        ::WaitMessage();
        PollEvents();
    }


    void* Win32Driver::LoadLibrary(const char* path) const
    {
        if (path == nullptr)
        {
            return nullptr;
        }

        // Add folder to search path to load dependency libraries
        auto folder = std::filesystem::path(path).parent_path();

        if (!folder.empty() && !folder.is_relative())
        {
            ::AddDllDirectory(folder.wstring().c_str());
        }

        const DWORD errorMode = SEM_NOOPENFILEERRORBOX;
        DWORD prevErrorMode = 0;
        const BOOL hasErrorMode = SetThreadErrorMode(errorMode, &prevErrorMode);

        ::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);

        auto widepath = Parse::ToWideString(path, strlen(path));
        void* handle = ::LoadLibraryW(widepath.data());

        if (!handle)
        {
            return nullptr;
        }

        if (hasErrorMode)
        {
            ::SetThreadErrorMode(prevErrorMode, nullptr);
        }

        return handle;
    }

    void Win32Driver::FreeLibrary(void* handle) const
    {
        if (handle)
        {
            ::FreeLibrary((HMODULE)handle);
        }
    }

    bool Win32Driver::GetHasFocus() const
    {
        DWORD foregroundProcess;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        return foregroundProcess == ::GetCurrentProcessId();
    }

    int2 Win32Driver::GetDesktopSize() const
    {
        return
        {
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN)
        };
    }

    int4 Win32Driver::GetMonitorRect(const int2& point, bool preferPrimary) const
    {
        auto monitor = (HMONITOR)GetNativeMonitorHandle(point, preferPrimary);

        MONITORINFO mi = { sizeof(mi) };
        ::GetMonitorInfoW(monitor, &mi);

        return
        {
            mi.rcWork.left,
            mi.rcWork.top,
            mi.rcWork.right - mi.rcWork.left,
            mi.rcWork.bottom - mi.rcWork.top
        };
    }

    void* Win32Driver::GetNativeMonitorHandle(const int2& point, bool preferPrimary) const
    {
        POINT pt{ point.x, point.y };
        return ::MonitorFromPoint(pt, preferPrimary ? MONITOR_DEFAULTTOPRIMARY : MONITOR_DEFAULTTONEAREST);
    }


    PlatformWindow* Win32Driver::CreateWindow(const PlatformWindowDescriptor& descriptor)
    {
        auto window = new Win32Window(descriptor);
        window->GetNext() = m_windowHead;
        m_windowHead = window;

        if (m_inputHandler)
        {
            m_inputHandler->InputHandler_OnConnect(window);
        }

        return window;
    }

    void Win32Driver::DestroyWindow(PlatformWindow* window)
    {
        if (m_inputHandler)
        {
            m_inputHandler->InputHandler_OnDisconnect(window);
        }

        auto head = m_windowHead;
        auto link = &m_windowHead;

        while (head)
        {
            if (head == window)
            {
                *link = head->GetNext();
                break;
            }

            link = &head->GetNext();
            head = head->GetNext();
        }

        delete static_cast<Win32Window*>(window);
    }

    void Win32Driver::SetInputHandler(InputHandler* handler)
    {
        m_inputHandler = handler;

        if (m_inputHandler)
        {
            auto head = m_windowHead;

            while (head)
            {
                m_inputHandler->InputHandler_OnConnect(head);
                head = head->GetNext();
            }
        }
    }


    std::string Win32Driver::GetClipboardString()
    {
        HANDLE object = NULL;
        WCHAR* buffer = NULL;
        auto tries = 0;

        while (!OpenClipboard(m_windowInstanceHelper))
        {
            Sleep(1);
            tries++;

            if (tries++ >= 10)
            {
                return std::string();
            }
        }

        object = GetClipboardData(CF_UNICODETEXT);

        if (!object)
        {
            ::CloseClipboard();
            return std::string();
        }

        buffer = (WCHAR*)::GlobalLock(object);

        if (!buffer)
        {
            ::CloseClipboard();
            return std::string();
        }

        std::string string = Parse::FromWideString(buffer, wcsnlen(buffer, 0xFFFFu));

        ::GlobalUnlock(object);
        ::CloseClipboard();

        return string;
    }

    void Win32Driver::SetClipboardString(const char* str)
    {
        HANDLE object = NULL;
        WCHAR* buffer = NULL;
        auto tries = 0;

        auto wstring = Parse::ToWideString(str, strlen(str));

        if (!wstring.empty())
        {
            return;
        }

        object = ::GlobalAlloc(GMEM_MOVEABLE, wstring.size() * sizeof(WCHAR));

        if (!object)
        {
            return;
        }

        buffer = (WCHAR*)::GlobalLock(object);

        if (!buffer)
        {
            ::GlobalFree(object);
            return;
        }

        ::GlobalUnlock(object);

        while (!OpenClipboard(m_windowInstanceHelper))
        {
            Sleep(1);
            tries++;

            if (tries++ >= 10)
            {
                GlobalFree(object);
                return;
            }
        }

        ::EmptyClipboard();
        ::SetClipboardData(CF_UNICODETEXT, object);
        ::CloseClipboard();
    }
    
    void Win32Driver::SetConsoleColor(uint32_t color) const
    {
        HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::SetConsoleTextAttribute(handle, (WORD)color);
    }

    void Win32Driver::SetConsoleVisible(bool value) const
    {
        HWND window = ::GetConsoleWindow();
        ::ShowWindow(window, value ? SW_SHOW : SW_HIDE);
    }

    bool Win32Driver::RemoteProcess(const char* executable, const char* arguments, std::string& outError) const
    {
        const auto executableLen = strlen(executable);
        const auto argumentsLen = strlen(arguments);

        if (executableLen == 0)
        {
            outError = "Execute remote processs executable path was empty";
            return false;
        }

        if (argumentsLen == 0)
        {
            outError = "Execute remote processs arguments were empty";
            return false;
        }

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        auto wideExecutable = Parse::ToWideString(executable, executableLen);
        auto wideArguments = Parse::ToWideString(arguments, argumentsLen);

        // Remove quotes from path
        if (wideExecutable[0] == L'\'')
        {
            wideExecutable = wideExecutable.substr(1);
        }

        if (wideExecutable.back() == L'\'')
        {
            wideExecutable = wideExecutable.substr(0, wideExecutable.size() - 1);
        }

        auto result = ::CreateProcessW(wideExecutable.data(), wideArguments.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        if (result == 0)
        {
            outError = Parse::FormatToString("Execute remote processs failed with error code: %lli", GetLastError());
            return false;
        }

        ::WaitForSingleObject(pi.hProcess, INFINITE);
        ::CloseHandle(pi.hProcess);

        return true;
    }


    bool Win32Driver::IsGreaterOSVersion(WORD major, WORD minor, WORD sp)
    {
        OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, 0, 0, {0}, sp };
        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
        ULONGLONG cond = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
        return PK_RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
    }

    bool Win32Driver::IsGreaterWin10Build(WORD build)
    {
        OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, build };
        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
        ULONGLONG cond = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        return PK_RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
    }

    bool Win32Driver::IsInputEvent(UINT uMsg)
    {
        return uMsg == WM_MOUSEMOVE ||
            uMsg == WM_CHAR ||
            uMsg == WM_SYSCHAR ||
            uMsg == WM_UNICHAR ||
            uMsg == WM_KEYDOWN ||
            uMsg == WM_SYSKEYDOWN ||
            uMsg == WM_KEYUP ||
            uMsg == WM_SYSKEYUP ||
            uMsg == WM_LBUTTONDOWN ||
            uMsg == WM_RBUTTONDOWN ||
            uMsg == WM_MBUTTONDOWN ||
            uMsg == WM_XBUTTONDOWN ||
            uMsg == WM_LBUTTONUP ||
            uMsg == WM_RBUTTONUP ||
            uMsg == WM_MBUTTONUP ||
            uMsg == WM_XBUTTONUP ||
            uMsg == WM_INPUT ||
            uMsg == WM_MOUSEWHEEL ||
            uMsg == WM_MOUSEHWHEEL ||
            uMsg == WM_DROPFILES;
    }

    bool Win32Driver::IsWindowEvent(UINT uMsg)
    {
        return uMsg == WM_MOUSEACTIVATE ||
            uMsg == WM_CAPTURECHANGED ||
            uMsg == WM_SETCURSOR ||
            uMsg == WM_MOUSELEAVE ||
            uMsg == WM_GETMINMAXINFO ||
            uMsg == WM_ENTERMENULOOP ||
            uMsg == WM_ENTERSIZEMOVE ||
            uMsg == WM_EXITMENULOOP ||
            uMsg == WM_EXITSIZEMOVE ||
            uMsg == WM_SIZE ||
            uMsg == WM_DPICHANGED ||
            uMsg == WM_MOVE ||
            uMsg == WM_SETFOCUS ||
            uMsg == WM_KILLFOCUS ||
            uMsg == WM_ACTIVATEAPP ||
            uMsg == WM_SYSCOMMAND ||
            uMsg == WM_SYSKEYDOWN ||
            uMsg == WM_CLOSE ||
            uMsg == WM_DESTROY ||
            uMsg == WM_ERASEBKGND ||
            uMsg == WM_CREATE ||
            uMsg == WM_NCACTIVATE ||
            uMsg == WM_NCPAINT;
    }


    InputKey Win32Driver::NativeToInputKey(int32_t native)
    {
        return Win32Driver::GetInstance()->native_to_keycode[native];
    }

    int32_t Win32Driver::InputKeyToNative(InputKey inputKey)
    {
        return Win32Driver::GetInstance()->keycode_to_native[(uint32_t)inputKey];
    }


    bool Win32Driver::IsDisabledCursorWindow(Win32Window* window)
    {
        return Win32Driver::GetInstance()->m_disabledCursorWindow == window;
    }

    RAWINPUT* Win32Driver::GetRawInput(Win32Window* window, LPARAM lParam)
    {
        auto driver = Win32Driver::GetInstance();

        if (driver->m_disabledCursorWindow != window)
        {
            return nullptr;
        }

        UINT size = 0u;
        ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

        if (size > driver->m_rawInputSize)
        {
            if (driver->m_rawInput)
            {
                free(driver->m_rawInput);
            }

            driver->m_rawInput = (RAWINPUT*)calloc(size, 1);
            driver->m_rawInputSize = size;
        }

        size = driver->m_rawInputSize;

        if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, driver->m_rawInput, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
        {
            return nullptr;
        }

        return driver->m_rawInput;
    }


    void Win32Driver::SetDisabledCursorWindow(Win32Window* window, bool value, const float2& restoreCursorPos)
    {
        auto driver = Win32Driver::GetInstance();

        if (window != driver->m_disabledCursorWindow && value)
        {
            driver->m_disabledCursorWindow = window;
            driver->m_restoreCursorPos = restoreCursorPos;
            return;
        }

        if (window == driver->m_disabledCursorWindow && !value)
        {
            driver->m_disabledCursorWindow = nullptr;
            window->SetCursorPosition(driver->m_restoreCursorPos);
        }
    }

    void Win32Driver::SetLockedCursorWindow(Win32Window* window, bool value)
    {
        auto driver = Win32Driver::GetInstance();

        if (value)
        {
            HWND handle = (HWND)window->GetNativeWindowHandle();
            RECT clipRect;
            ::GetClientRect(handle, &clipRect);
            ::ClientToScreen(handle, (POINT*)&clipRect.left);
            ::ClientToScreen(handle, (POINT*)&clipRect.right);
            ::ClipCursor(&clipRect);
            driver->m_lockedCursorWindow = window;
            return;
        }

        if (window == driver->m_lockedCursorWindow && !value)
        {
            ::ClipCursor(NULL);
            driver->m_lockedCursorWindow = nullptr;
        }
    }

    void Win32Driver::DispatchInputOnKey(InputDevice* device, InputKey key, bool isDown)
    {
        auto driver = Win32Driver::GetInstance();

        if (driver->m_inputHandler)
        {
            driver->m_inputHandler->InputHandler_OnKey(device, key, isDown);
        }
    }

    void Win32Driver::DispatchInputOnMouseMoved(InputDevice* device, const float2& position, const float2& size)
    {
        auto driver = Win32Driver::GetInstance();

        if (driver->m_inputHandler)
        {
            driver->m_inputHandler->InputHandler_OnMouseMoved(device, position, size);
        }
    }

    void Win32Driver::DispatchInputOnScroll(InputDevice* device, uint32_t axis, float offset)
    {
        auto driver = Win32Driver::GetInstance();

        if (driver->m_inputHandler)
        {
            driver->m_inputHandler->InputHandler_OnScroll(device, axis, offset);
        }
    }

    void Win32Driver::DispatchInputOnCharacter(InputDevice* device, uint32_t character)
    {
        auto driver = Win32Driver::GetInstance();

        if (driver->m_inputHandler && character >= 32 && (character <= 126 || character >= 160))
        {
            driver->m_inputHandler->InputHandler_OnCharacter(device, character);
        }
    }

    void Win32Driver::DispatchInputOnDrop(InputDevice* device, const char* const* paths, uint32_t count)
    {
        auto driver = Win32Driver::GetInstance();

        if (driver->m_inputHandler)
        {
            driver->m_inputHandler->InputHandler_OnDrop(device, paths, count);
        }
    }


    LRESULT Win32Driver::WindowProc_Helper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            case WM_INPUTLANGCHANGE:
            {
                //@TODO update input language
                break;
            }

            case WM_DISPLAYCHANGE:
            {
                // @TODO update monitors
                break;
            }

            case WM_DEVICECHANGE:
            {
                /*
                @TODO https://github.com/Ohjurot/DualSense-Windows/tree/main
                */
                break;
            }
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    LRESULT Win32Driver::WindowProc_Main(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Win32Window* window = (Win32Window*)::GetPropW(hWnd, Win32Window::WINDOW_PROP);
        
        // Happens on create before the prop is assigned.
        if (!window && uMsg == WM_NCCREATE && PK_PLATFORM_WINDOWS_IS_10_1607_OR_GREATER())
        {
            const CREATESTRUCTW* cs = (const CREATESTRUCTW*)lParam;
            const bool* useDpiScaling = (const bool*)cs->lpCreateParams;

            if (useDpiScaling && *useDpiScaling)
            {
                PK_EnableNonClientDpiScaling(hWnd);
            }
        }

        if (window && (Win32Driver::IsInputEvent(uMsg) || Win32Driver::IsWindowEvent(uMsg)))
        {
            return window->WindowProc(uMsg, wParam, lParam);
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

#endif
