#pragma once
#include "Core/Platform/PlatformInterfaces.h"

#if PK_PLATFORM_WINDOWS

// Windows... :)
#define far
#define near

#include <filesystem>
#include <dinput.h>
#include <xinput.h>
#include <dbt.h>
#include "shellapi.h"
#include "Core/Utilities/Parse.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/Ref.h"
#include "Core/Platform/Windows/Win32WindowManager.h"

// Define macros that some windows.h variants don't
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
#ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
#endif
#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif
#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif
#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR 0xFFFF
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif
#ifndef EDS_ROTATEDMODE
#define EDS_ROTATEDMODE 0x00000004
#endif
#ifndef DISPLAY_DEVICE_ACTIVE
#define DISPLAY_DEVICE_ACTIVE 0x00000001
#endif
#ifndef _WIN32_WINNT_WINBLUE
#define _WIN32_WINNT_WINBLUE 0x0603
#endif
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif
#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02e4
#endif
#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif
#ifndef OCR_HAND
#define OCR_HAND 32649
#endif

#ifndef DPI_ENUMS_DECLARED
typedef enum
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef enum
{
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif

// xinput.dll function pointer typedefs
typedef DWORD(WINAPI* PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef DWORD(WINAPI* PFN_XInputGetState)(DWORD, XINPUT_STATE*);

// dinput8.dll function pointer typedefs
typedef HRESULT(WINAPI* PFN_DirectInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

// user32.dll function pointer typedefs
typedef BOOL(WINAPI* PFN_SetProcessDPIAware)(void);
typedef BOOL(WINAPI* PFN_ChangeWindowMessageFilterEx)(HWND, UINT, DWORD, CHANGEFILTERSTRUCT*);
typedef BOOL(WINAPI* PFN_EnableNonClientDpiScaling)(HWND);
typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(HANDLE);
typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
typedef BOOL(WINAPI* PFN_AdjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
typedef int (WINAPI* PFN_GetSystemMetricsForDpi)(int, UINT);

// shcore.dll function pointer typedefs
typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

// ntdll.dll function pointer typedefs
typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);

#define PK_PLATFORM_WINDOWS_IS_8_OR_GREATER() PK::Win32Driver::IsGreaterOSVersion(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0)
#define PK_PLATFORM_WINDOWS_IS_8_1_OR_GREATER() PK::Win32Driver::IsGreaterOSVersion(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0)
#define PK_PLATFORM_WINDOWS_IS_10_1607_OR_GREATER() PK::Win32Driver::IsGreaterWin10Build(14393)
#define PK_PLATFORM_WINDOWS_IS_10_1703_OR_GREATER() PK::Win32Driver::IsGreaterWin10Build(15063)

#define PK_DirectInput8Create PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_DirectInput8Create
#define PK_XInputGetCapabilities PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_XInputGetCapabilities
#define PK_XInputGetState PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_XInputGetState
#define PK_SetProcessDPIAware PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_SetProcessDPIAware
#define PK_ChangeWindowMessageFilterEx PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_ChangeWindowMessageFilterEx
#define PK_EnableNonClientDpiScaling PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_EnableNonClientDpiScaling
#define PK_SetProcessDpiAwarenessContext PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_SetProcessDpiAwarenessContext
#define PK_GetDpiForWindow PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_GetDpiForWindow
#define PK_AdjustWindowRectExForDpi PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_AdjustWindowRectExForDpi
#define PK_GetSystemMetricsForDpi PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_GetSystemMetricsForDpi
#define PK_SetProcessDpiAwareness PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_SetProcessDpiAwareness
#define PK_GetDpiForMonitor PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_GetDpiForMonitor
#define PK_RtlVerifyVersionInfo PK::PlatformDriver::GetNative<Win32Driver>()->pkfn_RtlVerifyVersionInfo

namespace PK
{
    struct Win32Window;

    struct Win32Driver : public PlatformDriver
    {
        Win32Driver();
        ~Win32Driver();

        void PollEvents() final;
        void WaitEvents() final;

        inline void* GetProcess() const final { return instance; }
        inline void* GetHelperWindow() const final { return windowManager->GetHelperWindow(); }
        inline void* GetProcAddress(void* handle, const char* name) const final { return (void*)::GetProcAddress((HMODULE)handle, name); }

        void* LoadLibrary(const char* path) const final;
        void FreeLibrary(void* handle) const final;

        bool GetHasFocus() const final;
        int2 GetDesktopSize() const final;
        void* GetMonitorHandle(const int2& point, bool preferPrimary) const final;
        int4 GetMonitorRect(const int2& point, bool preferPrimary) const final;

        PlatformWindow* CreateWindow(const PlatformWindowDescriptor& descriptor) final;
        void DestroyWindow(PlatformWindow* window) final;

        void SetConsoleColor(uint32_t color) const final;
        void SetConsoleVisible(bool value) const final;
        bool RemoteProcess(const char* executable, const char* arguments, std::string& outError) const final;

        static bool IsGreaterOSVersion(WORD major, WORD minor, WORD sp);
        static bool IsGreaterWin10Build(WORD build);

        HINSTANCE instance = NULL;

        Unique<Win32WindowManager> windowManager = nullptr;

        void* dinput8_handle = nullptr;
        IDirectInput8W* dinput8_api = nullptr;
        PFN_DirectInput8Create pkfn_DirectInput8Create = nullptr;

        void* xinput_handle = nullptr;
        PFN_XInputGetCapabilities pkfn_XInputGetCapabilities = nullptr;
        PFN_XInputGetState pkfn_XInputGetState = nullptr;

        void* user32_handle = nullptr;
        PFN_SetProcessDPIAware pkfn_SetProcessDPIAware = nullptr;
        PFN_ChangeWindowMessageFilterEx pkfn_ChangeWindowMessageFilterEx = nullptr;
        PFN_EnableNonClientDpiScaling pkfn_EnableNonClientDpiScaling = nullptr;
        PFN_SetProcessDpiAwarenessContext pkfn_SetProcessDpiAwarenessContext = nullptr;
        PFN_GetDpiForWindow pkfn_GetDpiForWindow = nullptr;
        PFN_AdjustWindowRectExForDpi pkfn_AdjustWindowRectExForDpi = nullptr;
        PFN_GetSystemMetricsForDpi pkfn_GetSystemMetricsForDpi = nullptr;

        void* shcore_handle = nullptr;
        PFN_SetProcessDpiAwareness pkfn_SetProcessDpiAwareness = nullptr;
        PFN_GetDpiForMonitor pkfn_GetDpiForMonitor = nullptr;

        void* ntdll_handle = nullptr;
        PFN_RtlVerifyVersionInfo pkfn_RtlVerifyVersionInfo = nullptr;
    };
}

#endif
