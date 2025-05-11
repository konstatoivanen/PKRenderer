#include "PrecompiledHeader.h"

#if PK_PLATFORM_WINDOWS
#include "Win32Driver.h"

namespace PK
{
    static int s_localPtr;

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
        pkfn_GetSystemMetricsForDpi = (PFN_GetSystemMetricsForDpi)GetProcAddress(user32_handle, "GetSystemMetricsForDpi");

        if (dinput8_handle)
        {
            pkfn_DirectInput8Create = (PFN_DirectInput8Create)GetProcAddress(dinput8_handle, "DirectInput8Create");
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

        windowManager = CreateUnique<Win32WindowManager>(instance);
    }

    Win32Driver::~Win32Driver()
    {
        windowManager = nullptr;
        FreeLibrary(user32_handle);
        FreeLibrary(dinput8_handle);
        FreeLibrary(shcore_handle);
        FreeLibrary(ntdll_handle);
        FreeLibrary(xinput_handle);
    }

    
    void Win32Driver::PollEvents()
    {
        if (windowManager)
        {
            windowManager->PollEvents();
        }
    }

    void Win32Driver::WaitEvents()
    {
        if (windowManager)
        {
            windowManager->WaitEvents();
        }
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

    void* Win32Driver::GetMonitorHandle(const int2& point, bool preferPrimary) const
    {
        POINT pt{ point.x, point.y };
        return ::MonitorFromPoint(pt, preferPrimary ? MONITOR_DEFAULTTOPRIMARY : MONITOR_DEFAULTTONEAREST);
    }

    int4 Win32Driver::GetMonitorRect(const int2& point, bool preferPrimary) const
    {
        auto monitor = (HMONITOR)GetMonitorHandle(point, preferPrimary);

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

    PlatformWindow* Win32Driver::CreateWindow(const PlatformWindowDescriptor& descriptor)
    {
        return windowManager ? windowManager->CreateWindow(descriptor) : nullptr;
    }

    void Win32Driver::DestroyWindow(PlatformWindow* window)
    {
        if (windowManager)
        {
            windowManager->DestroyWindow(window);
        }
    }

    
    void Win32Driver::SetConsoleColor(uint32_t color) const
    {
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(handle, (WORD)color);
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

        auto result = CreateProcessW(wideExecutable.data(), wideArguments.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        if (result == 0)
        {
            outError = Parse::FormatToString("Execute remote processs failed with error code: %lli", GetLastError());
            return false;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);

        return true;
    }


    bool Win32Driver::IsGreaterOSVersion(WORD major, WORD minor, WORD sp)
    {
        OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, 0, 0, {0}, sp };
        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
        ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
        cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
        cond = VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
        return PK_RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
    }

    bool Win32Driver::IsGreaterWin10Build(WORD build)
    {
        OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, build };
        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
        ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
        cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
        cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        return PK_RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
    }
}

#endif
