#include "PrecompiledHeader.h"

#if PK_PLATFORM_WINDOWS
#include "Win32Internal.h"
#include "Core/Utilities/FixedString.h"

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
    static int64_t s_programMemoryExclusive = 0ll;

    // These didnt want to get included from hidclass.h for some reason
    static const GUID s_GUID_DEVINTERFACE_HID = { 0x4d1e55b2,0xf16f,0x11cf,{0x88,0xcb,0x00,0x11,0x11,0x00,0x00,0x30} };
    static const GUID s_IID_IDirectInput8W = { 0xbf798031,0x483a,0x4da2,{0xaa,0x99,0x5d,0x64,0xed,0x36,0x97,0x00} };

    int Win32Platform::Initialize()
    {
        if (resources)
        {
            return -1;
        }

        resources = Memory::New<Win32Resources>();

        if (!resources)
        {
            return -1;
        }

        LARGE_INTEGER frequency;
        if (::QueryPerformanceFrequency(&frequency))
        {
            resources->cyclesToSeconds = 1.0 / static_cast<double>(frequency.QuadPart);
        }

        DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
        Memory::Assert(GetModuleHandleExW(flags, (const WCHAR*)&s_localPtr, (HMODULE*)&resources->instance), "Failed to get program HINSTANCE");

        ::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

        resources->user32_handle = Platform::LoadLibrary("user32.dll");
        resources->dinput8_handle = LoadLibrary("dinput8.dll");
        resources->shcore_handle = LoadLibrary("shcore.dll");
        resources->ntdll_handle = LoadLibrary("ntdll.dll");
        resources->dwmapi_handle = LoadLibrary("dwmapi.dll");

        Memory::Assert(resources->user32_handle, "Failed to load user32.dll");

        pkfn_SetProcessDPIAware = (PFN_SetProcessDPIAware)GetProcAddress(resources->user32_handle, "SetProcessDPIAware");
        pkfn_ChangeWindowMessageFilterEx = (PFN_ChangeWindowMessageFilterEx)GetProcAddress(resources->user32_handle, "ChangeWindowMessageFilterEx");
        pkfn_EnableNonClientDpiScaling = (PFN_EnableNonClientDpiScaling)GetProcAddress(resources->user32_handle, "EnableNonClientDpiScaling");
        pkfn_SetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(resources->user32_handle, "SetProcessDpiAwarenessContext");
        pkfn_GetDpiForWindow = (PFN_GetDpiForWindow)GetProcAddress(resources->user32_handle, "GetDpiForWindow");
        pkfn_AdjustWindowRectExForDpi = (PFN_AdjustWindowRectExForDpi)GetProcAddress(resources->user32_handle, "AdjustWindowRectExForDpi");

        if (resources->dinput8_handle)
        {
            pkfn_DirectInput8Create = (PFN_DirectInput8Create)GetProcAddress(resources->dinput8_handle, "DirectInput8Create");

            if (FAILED(PK_DirectInput8Create(resources->instance, DIRECTINPUT_VERSION, s_IID_IDirectInput8W, (void**)&resources->dinput8_api, NULL)))
            {
                resources->dinput8_api = nullptr;
            }
        }

        if (resources->shcore_handle)
        {
            pkfn_SetProcessDpiAwareness = (PFN_SetProcessDpiAwareness)GetProcAddress(resources->shcore_handle, "SetProcessDpiAwareness");
            pkfn_GetDpiForMonitor = (PFN_GetDpiForMonitor)GetProcAddress(resources->shcore_handle, "GetDpiForMonitor");
        }

        if (resources->ntdll_handle)
        {
            pkfn_RtlVerifyVersionInfo = (PFN_RtlVerifyVersionInfo)GetProcAddress(resources->ntdll_handle, "RtlVerifyVersionInfo");
        }

        if (resources->dwmapi_handle)
        {
            pkfn_DwmSetWindowAttribute = (PFN_DwmSetWindowAttribute)GetProcAddress(resources->dwmapi_handle, "DwmSetWindowAttribute");
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
            resources->xinput_handle = LoadLibrary(xinputModuleNames[i]);

            if (resources->xinput_handle)
            {
                pkfn_XInputGetCapabilities = (PFN_XInputGetCapabilities)GetProcAddress(resources->xinput_handle, "XInputGetCapabilities");
                pkfn_XInputGetState = (PFN_XInputGetState)GetProcAddress(resources->xinput_handle, "XInputGetState");
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
            wc.hInstance = resources->instance;
            wc.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
            wc.lpszClassName = Win32Window::CLASS_HELPER;
            Memory::Assert((resources->windowClassHelper = ::RegisterClassExW(&wc)), "Failed to create window helper class.");
        }

        // Main window class for actual display windows.
        {
            WNDCLASSEXW wc = { sizeof(wc) };
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            wc.lpfnWndProc = WindowProc_Main;
            wc.hInstance = resources->instance;
            wc.hCursor = ::LoadCursorW(NULL, IDC_ARROW);
            wc.lpszClassName = Win32Window::CLASS_MAIN;
            wc.hIcon = (HICON)::LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
            Memory::Assert((resources->windowClassMain = ::RegisterClassExW(&wc)), "Failed to create window main class.");
        }

        resources->windowInstanceHelper = ::CreateWindowExW
        (
            WS_EX_OVERLAPPEDWINDOW,
            Win32Window::CLASS_HELPER,
            L"PK_PLATFORM_WIN32_WINDOW_HELPER",
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 1, 1,
            NULL, NULL,
            resources->instance,
            NULL
        );

        Memory::Assert(resources->windowInstanceHelper, "Failed to create helper window.");

        ::ShowWindow(resources->windowInstanceHelper, SW_HIDE);

        #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
        {
            DEV_BROADCAST_DEVICEINTERFACE_W dbi;
            ZeroMemory(&dbi, sizeof(dbi));
            dbi.dbcc_size = sizeof(dbi);
            dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            dbi.dbcc_classguid = s_GUID_DEVINTERFACE_HID;
            resources->deviceNotificationHandle = ::RegisterDeviceNotificationW(resources->windowInstanceHelper, (DEV_BROADCAST_HDR*)&dbi, DEVICE_NOTIFY_WINDOW_HANDLE);
        }
        #endif

        {
            auto& native_to_keycode = resources->native_to_keycode;
            auto& keycode_to_native = resources->keycode_to_native;

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

        return 0;
    }

    int Win32Platform::Terminate()
    {
        auto head = resources->windowHead;

        while (head)
        {
            resources->windowHead = head->GetNext();

            if (resources->inputHandler)
            {
                resources->inputHandler->InputHandler_OnDisconnect(head);
            }

            Memory::Delete(head);
            head = resources->windowHead;
        }

        ::ClipCursor(NULL);

        if (resources->deviceNotificationHandle)
        {
            ::UnregisterDeviceNotification(resources->deviceNotificationHandle);
        }

        if (resources->windowInstanceHelper)
        {
            ::DestroyWindow((HWND)resources->windowInstanceHelper);
        }

        if (resources->windowClassHelper)
        {
            ::UnregisterClassW(Win32Window::CLASS_HELPER, nullptr);
            resources->windowClassHelper = NULL;
        }

        if (resources->windowClassMain)
        {
            ::UnregisterClassW(Win32Window::CLASS_MAIN, nullptr);
            resources->windowClassMain = NULL;
        }

        Memory::Free(resources->rawInput);
        Memory::Free(resources->clipboard);
        resources->rawInput = nullptr;
        resources->clipboard = nullptr;

        if (resources->dinput8_api)
        {
            IDirectInput8_Release(resources->dinput8_api);
            resources->dinput8_api = nullptr;
        }

        FreeLibrary(resources->user32_handle);
        FreeLibrary(resources->dinput8_handle);
        FreeLibrary(resources->shcore_handle);
        FreeLibrary(resources->ntdll_handle);
        FreeLibrary(resources->dwmapi_handle);
        FreeLibrary(resources->xinput_handle);

        Memory::Free(resources);
        resources = nullptr;

        return 0;
    }

    void* Win32Platform::AllocateAligned(size_t size, size_t alignment)
    {
        auto ptr = _aligned_malloc(size, alignment);
    
        if (!ptr)
        {
            Platform::FatalExit("Out of memory");
        }

        _InlineInterlockedAdd64(reinterpret_cast<volatile int64_t*>(&s_programMemoryExclusive), size);

        return ptr;
    }

    void Win32Platform::FreeAligned(void* block)
    {
        if (block)
        {
            auto size = -(int64_t)_aligned_msize(block, 16ull, 0ull);
            _InlineInterlockedAdd64(reinterpret_cast<volatile int64_t*>(&s_programMemoryExclusive), size);
            _aligned_free(block);
        }
    }

    PlatformMemoryInfo Win32Platform::GetMemoryInfo()
    {
        MEMORYSTATUSEX statusx;
        statusx.dwLength = sizeof(statusx);
        ::GlobalMemoryStatusEx(&statusx);

        PROCESS_MEMORY_COUNTERS_EX countersEx;
        countersEx.cb = sizeof(countersEx);
        ::GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&countersEx, sizeof(countersEx));

        PlatformMemoryInfo info{};
        info.physicalMemoryTotal = statusx.ullTotalPhys;
        info.physicalMemoryUsed = statusx.ullTotalPhys - statusx.ullAvailPhys;
        info.virtualMemoryTotal = statusx.ullTotalVirtual;
        info.virtualMemoryUsed = statusx.ullTotalVirtual - statusx.ullAvailVirtual;
        info.programMemoryUsedInclusive = countersEx.WorkingSetSize;
        info.programMemoryUsedExclusive = s_programMemoryExclusive;
        return info;
    }


    void Win32Platform::PollEvents(bool wait)
    {
        if (wait)
        {
            ::WaitMessage();
        }

        auto activeHandle = ::GetActiveWindow();
        auto activeWindow = activeHandle ? (Win32Window*)GetPropW(activeHandle, Win32Window::WINDOW_PROP) : nullptr;
        auto inputHandler = resources->inputHandler;

        // Dont dispatch poll events when we're waiting for an activation window event.
        if (!wait && inputHandler)
        {
            inputHandler->InputHandler_OnPoll();
        }

        if (!wait && inputHandler && activeWindow)
        {
            inputHandler->InputHandler_OnPoll(activeWindow);
        }

        MSG msg;
        while (::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                auto head = resources->windowHead;

                while (head)
                {
                    head->Close();
                    head = head->GetNext();
                }
            }
            else
            {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
        }

        if (activeWindow)
        {
            activeWindow->OnPollEvents();
        }

        if (resources->disabledCursorWindow)
        {
            resources->disabledCursorWindow->SetCursorPosToCenter();
        }
    }

    void* Win32Platform::GetProcess() { return resources->instance; }
    void* Win32Platform::GetHelperWindow() { return resources->windowInstanceHelper; }
    void* Win32Platform::GetProcAddress(void* handle, const char* name) { return (void*)::GetProcAddress((HMODULE)handle, name); }

    double Win32Platform::GetTimeSeconds()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return double(counter.QuadPart) * resources->cyclesToSeconds;
    }

    uint64_t Win32Platform::GetTimeCycles()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }

    void* Win32Platform::LoadLibrary(const char* path)
    {
        if (!path || !path[0])
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

        FixedWString256 widepath(strlen(path), path);
        void* handle = ::LoadLibraryW(widepath);

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

    void Win32Platform::FreeLibrary(void* handle)
    {
        if (handle)
        {
            ::FreeLibrary((HMODULE)handle);
        }
    }


    bool Win32Platform::GetHasFocus()
    {
        DWORD foregroundProcess;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        return foregroundProcess == ::GetCurrentProcessId();
    }

    int2 Win32Platform::GetDesktopSize()
    {
        return
        {
            GetSystemMetrics(SM_CXSCREEN),
            GetSystemMetrics(SM_CYSCREEN)
        };
    }

    int4 Win32Platform::GetMonitorRect(const int2& point, bool preferPrimary)
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

    void* Win32Platform::GetNativeMonitorHandle(const int2& point, bool preferPrimary)
    {
        POINT pt{ point.x, point.y };
        return ::MonitorFromPoint(pt, preferPrimary ? MONITOR_DEFAULTTOPRIMARY : MONITOR_DEFAULTTONEAREST);
    }

    PlatformWindow* Win32Platform::CreateWindow(const PlatformWindowDescriptor& descriptor)
    {
        auto window = Memory::New<Win32Window>(descriptor);
        window->GetNext() = resources->windowHead;
        resources->windowHead = window;

        if (resources->inputHandler)
        {
            resources->inputHandler->InputHandler_OnConnect(window);
        }

        return window;
    }

    void Win32Platform::DestroyWindow(PlatformWindow* window)
    {
        if (resources->inputHandler)
        {
            resources->inputHandler->InputHandler_OnDisconnect(window);
        }

        auto head = resources->windowHead;
        auto link = &resources->windowHead;

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

        Memory::Delete(static_cast<Win32Window*>(window));
    }

    void Win32Platform::SetInputHandler(InputHandler* handler)
    {
        resources->inputHandler = handler;

        if (resources->inputHandler)
        {
            auto head = resources->windowHead;

            while (head)
            {
                resources->inputHandler->InputHandler_OnConnect(head);
                head = head->GetNext();
            }
        }
    }


    const char* Win32Platform::GetClipboardString()
    {
        const char* result = nullptr;

        if (::OpenClipboard(resources->windowInstanceHelper))
        {
            auto object = GetClipboardData(CF_UNICODETEXT);

            if (object)
            {
                auto buffer = static_cast<WCHAR*>(::GlobalLock(object));
                auto length = wcsnlen(buffer, 0xFFFFu) + 1u;
                auto size = length * sizeof(WCHAR);

                if (!resources->clipboard || resources->clipboardSize < size)
                {
                    Memory::Free(resources->clipboard);
                    resources->clipboard = Memory::AllocateClear<char>(size);
                    resources->clipboardSize = size;
                    resources->clipboard[size] = '\0';
                }

                result = resources->clipboard;
                String::ToNarrow(resources->clipboard, buffer, length);
                ::GlobalUnlock(object);
            }
            else if (!object)
            {
                object = ::GetClipboardData(CF_TEXT);

                if (object)
                {
                    auto buffer = static_cast<char*>(::GlobalLock(object));
                    auto length = strnlen(buffer, 0xFFFFu) + 1u;
                    auto size = length;

                    if (!resources->clipboard || resources->clipboardSize < size)
                    {
                        Memory::Free(resources->clipboard);
                        resources->clipboard = Memory::AllocateClear<char>(size);
                        resources->clipboardSize = size;
                        resources->clipboard[size] = '\0';
                    }

                    result = resources->clipboard;
                    String::Copy(resources->clipboard, buffer, length - 1u);
                    ::GlobalUnlock(object);
                }
            }

            ::CloseClipboard();
        }

        return result ? result : "";
    }

    void Win32Platform::SetClipboardString(const char* str)
    {
        auto length = strlen(str);

        if (length > 0)
        {
            const auto object = ::GlobalAlloc(GMEM_MOVEABLE, (length + 1ull) * sizeof(WCHAR));
            
            if (object)
            {
                auto buffer = static_cast<WCHAR*>(::GlobalLock(object));
                String::ToWide(buffer, str, length + 1ull);
                buffer[length] = '\0';
                ::GlobalUnlock(object);
                
                if (!::OpenClipboard(resources->windowInstanceHelper))
                {
                    ::GlobalFree(object);
                    return;
                }

                ::EmptyClipboard();
                ::SetClipboardData(CF_UNICODETEXT, object);
                ::CloseClipboard();
            }
        }
    }
    
    void Win32Platform::SetConsoleColor(uint32_t color)
    {
        HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        ::SetConsoleTextAttribute(handle, (WORD)color);
    }

    void Win32Platform::SetConsoleVisible(bool value)
    {
        HWND window = ::GetConsoleWindow();
        ::ShowWindow(window, value ? SW_SHOW : SW_HIDE);
    }

    uint32_t Win32Platform::RemoteProcess(const char* executable, const char* arguments)
    {
        if (!executable || !executable[0] || !arguments || !arguments[0])
        {
            return 1u;
        }

        auto executableLen = strlen(executable);
        auto argumentsLen = strlen(arguments);

        // Remove quotes from path
        if (executable[executableLen - 1ull] == '\'')
        {
            executableLen--;
        }

        if (executable[0] == '\'')
        {
            executableLen--;
            executable++;
        }

        if (executableLen == 0ull)
        {
            return 1u;
        }

        FixedWString512 wideExecutable(executableLen, executable);
        FixedWString512 wideArguments(argumentsLen, arguments);

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        auto result = ::CreateProcessW(wideExecutable, wideArguments, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        if (result == 0)
        {
            return ::GetLastError();
        }

        ::WaitForSingleObject(pi.hProcess, INFINITE);
        ::CloseHandle(pi.hProcess);
        return 0u;
    }


    bool Win32Platform::IsGreaterOSVersion(WORD major, WORD minor, WORD sp)
    {
        OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, 0, 0, {0}, sp };
        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
        ULONGLONG cond = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
        return PK_RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
    }

    bool Win32Platform::IsGreaterWin10Build(WORD build)
    {
        OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, build };
        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
        ULONGLONG cond = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
        cond = ::VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        return PK_RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
    }

    bool Win32Platform::IsInputEvent(UINT uMsg)
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

    bool Win32Platform::IsWindowEvent(UINT uMsg)
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


    InputKey Win32Platform::NativeToInputKey(int32_t native)
    {
        return resources->native_to_keycode[native];
    }

    int32_t Win32Platform::InputKeyToNative(InputKey inputKey)
    {
        return resources->keycode_to_native[(uint32_t)inputKey];
    }


    bool Win32Platform::IsDisabledCursorWindow(Win32Window* window)
    {
        return resources->disabledCursorWindow == window;
    }

    RAWINPUT* Win32Platform::GetRawInput(Win32Window* window, LPARAM lParam)
    {
        if (resources->disabledCursorWindow != window)
        {
            return nullptr;
        }

        UINT size = 0u;
        ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

        if (size > resources->rawInputSize)
        {
            if (resources->rawInput)
            {
                Memory::Free(resources->rawInput);
            }

            resources->rawInput = Memory::AllocateClear<RAWINPUT>(1u);
            resources->rawInputSize = size;
        }

        if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, resources->rawInput, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
        {
            return nullptr;
        }

        return resources->rawInput;
    }


    void Win32Platform::SetDisabledCursorWindow(Win32Window* window, bool value, const float2& restoreCursorPos)
    {
        if (window != resources->disabledCursorWindow && value)
        {
            resources->disabledCursorWindow = window;
            resources->restoreCursorPos = restoreCursorPos;
            return;
        }

        if (window == resources->disabledCursorWindow && !value)
        {
            resources->disabledCursorWindow = nullptr;
            window->SetCursorPosition(resources->restoreCursorPos);
        }
    }

    void Win32Platform::SetLockedCursorWindow(Win32Window* window, bool value)
    {
        if (value)
        {
            HWND handle = (HWND)window->GetNativeWindowHandle();
            RECT clipRect;
            ::GetClientRect(handle, &clipRect);
            ::ClientToScreen(handle, (POINT*)&clipRect.left);
            ::ClientToScreen(handle, (POINT*)&clipRect.right);
            ::ClipCursor(&clipRect);
            resources->lockedCursorWindow = window;
            return;
        }

        if (window == resources->lockedCursorWindow && !value)
        {
            ::ClipCursor(NULL);
            resources->lockedCursorWindow = nullptr;
        }
    }

    void Win32Platform::DispatchInputOnKey(InputDevice* device, InputKey key, bool isDown)
    {
        if (resources->inputHandler)
        {
            resources->inputHandler->InputHandler_OnKey(device, key, isDown);
        }
    }

    void Win32Platform::DispatchInputOnMouseMoved(InputDevice* device, const float2& position, const float2& size)
    {
        if (resources->inputHandler)
        {
            resources->inputHandler->InputHandler_OnMouseMoved(device, position, size);
        }
    }

    void Win32Platform::DispatchInputOnScroll(InputDevice* device, uint32_t axis, float offset)
    {
        if (resources->inputHandler)
        {
            resources->inputHandler->InputHandler_OnScroll(device, axis, offset);
        }
    }

    void Win32Platform::DispatchInputOnCharacter(InputDevice* device, uint32_t character)
    {
        if (resources->inputHandler && character >= 32 && (character <= 126 || character >= 160))
        {
            resources->inputHandler->InputHandler_OnCharacter(device, character);
        }
    }

    void Win32Platform::DispatchInputOnDrop(InputDevice* device, const char* const* paths, uint32_t count)
    {
        if (resources->inputHandler)
        {
            resources->inputHandler->InputHandler_OnDrop(device, paths, count);
        }
    }


    LRESULT Win32Platform::WindowProc_Helper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

    LRESULT Win32Platform::WindowProc_Main(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

        if (window && (Win32Platform::IsInputEvent(uMsg) || Win32Platform::IsWindowEvent(uMsg)))
        {
            return window->WindowProc(uMsg, wParam, lParam);
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

#endif
