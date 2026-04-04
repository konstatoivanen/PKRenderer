#pragma once
#if PK_IS_PLATFORM_HEADER && PK_PLATFORM_WINDOWS

#if defined(_M_X64)
#define PK_PLATFORM_X64 1
#endif

#if defined(_M_ARM64)
#define PK_PLATFORM_ARM64 1
#endif

#define PK_PLATFORM_DEBUG_BREAK __debugbreak()
#define VK_USE_PLATFORM_WIN32_KHR

// Defined in PKRenderer.rc
#define PK_WIN32_EMBEDDED_ICON_NAME L"MAINICON"

// Min Windows 7
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS WINVER
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
//#define NOGDI
#define NODRAWTEXT
//#define NOCTLMGR
#define NOFLATSBAPIS

#define NOGDICAPMASKS
//#define NOSYSMETRICS
#define NOMENUS
//#define NOICONS
//#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
//#define NOCLIPBOARD
#define NOCOLOR
#define NOKERNEL
#define NONLS
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE
#define STRICT

#include <Windows.h>

#undef MemoryBarrier
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#undef CreateDirectory
#undef GetComputerName
#undef GetUserName
#undef MessageBox
#undef GetCommandLine
#undef CreateWindow
#undef CreateProcess
#undef SetWindowText
#undef DrawText
#undef CreateFont
#undef IsMinimized
#undef IsMaximized
#undef LoadIcon
#undef InterlockedOr
#undef InterlockedAnd
#undef InterlockedExchange
#undef InterlockedCompareExchange
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedAdd
#undef GetObject
#undef GetClassName
#undef GetMessage
#undef CreateMutex
#undef DrawState
#undef LoadLibrary
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable

#undef far
#undef near

struct IDirectInput8W;

namespace PK
{
    struct Win32Window;
    struct Win32Resources;
    struct InputDevice;
    enum class InputKey;

    struct Win32Platform : public IPlatform 
    {
        using IPlatform::PollEvents;
        using IPlatform::WaitEvents;

        static int Initialize();
        static int Terminate();

        static void* AllocateAligned(size_t size, size_t alignment);
        static void FreeAligned(void* block);
        static PlatformMemoryInfo GetMemoryInfo();

        static void PollEvents(bool wait);

        static void* GetProcess();
        static void* GetHelperWindow();
        static void* GetProcAddress(void* handle, const char* name);
        static void* LoadLibrary(const char* path);
        static void FreeLibrary(void* handle);

        static bool GetHasFocus();
        static int2 GetDesktopSize();
        static int4 GetMonitorRect(const int2& point, bool preferPrimary);
        static void* GetNativeMonitorHandle(const int2& point, bool preferPrimary);

        static double GetTimeSeconds();
        static uint64_t GetTimeCycles();

        static PlatformWindow* CreateWindow(const PlatformWindowDescriptor& descriptor);
        static void DestroyWindow(PlatformWindow* window);

        static void SetInputHandler(InputHandler* handler);

        static const char* GetClipboardString();
        static void SetClipboardString(const char* str);

        static void SetConsoleColor(uint32_t color);
        static void SetConsoleVisible(bool value);
        static uint32_t RemoteProcess(const char* executable, const char* arguments);

        inline static uint32_t InterlockedExchange(volatile uint32_t* dst, uint32_t exchange) { return _InterlockedExchange(dst, exchange); }
        inline static uint32_t InterlockedCompareExchange(volatile uint32_t* dst, uint32_t exchange, uint32_t comperand) { return _InterlockedCompareExchange(dst, exchange, comperand); }
        inline static uint32_t InterlockedAdd(volatile uint32_t* dst, uint32_t value) { return _InterlockedExchangeAdd(dst, value); }
        inline static uint32_t InterlockedIncrement(volatile uint32_t* dst) { return _InterlockedIncrement(dst); }
        inline static uint32_t InterlockedDecrement(volatile uint32_t* dst) { return _InterlockedDecrement(dst); }
        inline static uint32_t AtomicRead(const volatile uint32_t* dst) { return (unsigned)__iso_volatile_load32(reinterpret_cast<const volatile int32_t*>(dst)); }
        inline static void AtomicStore(uint32_t volatile* dst, uint32_t value) { __iso_volatile_store32(reinterpret_cast<volatile int32_t*>(dst), value); }
        inline static uint64_t BitScan64(uint64_t mask) { auto index = 0ul; return _BitScanForward64(&index, mask) ? index : 64u; }

        static bool IsGreaterOSVersion(WORD major, WORD minor, WORD sp);
        static bool IsGreaterWin10Build(WORD build);
        static bool IsInputEvent(UINT uMsg);
        static bool IsWindowEvent(UINT uMsg);

        static InputKey NativeToInputKey(int32_t native);
        static int32_t InputKeyToNative(InputKey inputKey);

    protected:
        friend struct Win32Window;

        static bool IsDisabledCursorWindow(Win32Window* window);
        static RAWINPUT* GetRawInput(Win32Window* window, LPARAM lParam);
        static void SetDisabledCursorWindow(Win32Window* window, bool value, const float2& restoreCursorPos);
        static void SetLockedCursorWindow(Win32Window* window, bool value);
        static void DispatchInputOnKey(InputDevice* device, InputKey key, bool isDown);
        static void DispatchInputOnMouseMoved(InputDevice* device, const float2& position, const float2& size);
        static void DispatchInputOnScroll(InputDevice* device, uint32_t axis, float offset);
        static void DispatchInputOnCharacter(InputDevice* device, uint32_t character);
        static void DispatchInputOnDrop(InputDevice* device, const char* const* paths, uint32_t count);

        static LRESULT CALLBACK WindowProc_Helper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK WindowProc_Main(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        inline static Win32Resources* resources;
    };
}

#endif
