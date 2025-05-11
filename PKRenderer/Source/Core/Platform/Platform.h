#pragma once
// Some manual guarding to avoid user error
#define PK_IS_PLATFORM_HEADER 1

// Compiler defines
#if __cplusplus < 201703L
    #error "C++ 17 support or newer required!"
#endif

#if defined(__clang__)
    #define PK_DLLEXPORT __attribute__ ((__visibility__ ("default")))
    #define PK_DLLIMPORT
    #define PK_THREADLOCAL __thread
    #define PK_STDCALL __attribute__((stdcall))
    #define PK_CDECL __attribute__((cdecl))
    #define PK_RESTRICT __restrict__
    #define PK_INLINE inline
    #define PK_FORCE_INLINE inline
    #define PK_FORCE_NOINLINE __attribute__((noinline))
    #define PK_NO_RETURN __attribute__((noreturn))
    #define PK_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
    #define PK_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
    #define PK_OFFSET_OF(X, Y) __builtin_offsetof(X, Y)
#elif defined(_MSC_VER)
    #if _MSVC_LANG < 201703L
        #error "MSVC with c++ 17 support or newer required!"
    #endif
    #define PK_DLLEXPORT __declspec(dllexport)
    #define PK_DLLIMPORT __declspec(dllimport)
    #define PK_THREADLOCAL __declspec(thread)
    #define PK_STDCALL __stdcall
    #define PK_CDECL __cdecl
    #define PK_RESTRICT __restrict
    #define PK_INLINE __inline
    #define PK_FORCE_INLINE __forceinline
    #define PK_FORCE_NOINLINE __declspec(noinline)
    #define PK_NO_RETURN __declspec(noreturn)
    #define PK_NO_SANITIZE_ADDRESS
    #define PK_NO_SANITIZE_THREAD
    #define PK_OFFSET_OF(X, Y) offsetof(X, Y)
    #undef __PRETTY_FUNCTION__
    #define __PRETTY_FUNCTION__ __FUNCSIG__
#else
    #error "Unsupported compiler!"
#endif

#if defined(_WIN32) && defined(_WIN64)
#define PK_PLATFORM_WINDOWS 1
#else
#define PK_PLATFORM_WINDOWS 0
#endif

#if defined(__linux__) && defined(_LINUX64)
#define PK_PLATFORM_LINUX 1
#else
#define PK_PLATFORM_LINUX 0
#endif

#include <stdint.h>
#include <string>
#include "Core/Math/MathFwd.h"

namespace PK
{
    struct PlatformDriver;
    struct PlatformWindow;
    struct PlatformWindowDescriptor;
    struct PlatformWindowListener;
    struct PlatformWindowInputListener;

    namespace Platform
    {
        PlatformDriver* CreateDriver();
        void DestroyDriver(PlatformDriver* driver);
        
        void PollEvents();
        void WaitEvents();

        void* GetProcess();
        void* GetHelperWindow();

        void* LoadLibrary(const char* path);
        void FreeLibrary(void* handle);
        void* GetProcAddress(void* handle, const char* name);

        bool GetHasFocus();
        int2 GetDesktopSize();
        void* GetMonitorHandle(const int2& point, bool preferPrimary);
        int4 GetMonitorRect(const int2& point, bool preferPrimary);

        PlatformWindow* CreateWindow(const PlatformWindowDescriptor& descriptor);
        void DestroyWindow(PlatformWindow* window);

        void SetConsoleColor(uint32_t color);
        void SetConsoleVisible(bool value);
        bool RemoteProcess(const char* executable, const char* arguments, std::string& outError);
    }
}

#include "Windows/Win32Platform.h"
#include "Linux/LinuxPlatform.h"

#ifndef PK_PLAFTORM_DRIVER_TYPE
#error "Platform driver is undefined!"
#endif

#ifndef PK_PLATFORM_ARCH_X64
#define PK_PLATFORM_ARCH_X64 0
#endif
#ifndef PK_PLATFORM_ARCH_ARM64
#define PK_PLATFORM_ARCH_ARM64 0
#endif

#ifndef PK_PLATFORM_DEBUG_BREAK
#define PK_PLATFORM_DEBUG_BREAK
#endif

#ifndef PK_PLATFORM_TEXT_IS_CHAR16
#define PK_PLATFORM_TEXT_IS_CHAR16 0
#endif

// SIMD defines
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
#define PK_PLATFORM_SIMD_SSE2 1
#if defined(__SSE3__)
#define PK_PLATFORM_SIMD_SSE3 1
#endif
#if defined(__SSE4__)
#define PK_PLATFORM_SIMD_SSE4 1
#endif
#if defined(__SSE4_1__)
#define PK_PLATFORM_SIMD_SSE4_1 1
#endif
#if defined(__SSE4_2__)
#define PK_PLATFORM_SIMD_SSE4_2 1
#endif
#endif

#if defined(_M_ARM) || defined(__ARM_NEON__) || defined(__ARM_NEON)
#define PK_PLATFORM_SIMD_NEON 1
#endif

#if defined(_M_PPC) || defined(__CELLOS_LV2__)
#define PK_PLATFORM_SIMD_VMX 1
#endif

#define PK_PLATFORM_SIMD (PK_PLATFORM_SIMD_SSE2 || PK_PLATFORM_SIMD_SSE3 || PK_PLATFORM_SIMD_SSE4 || PK_PLATFORM_SIMD_NEON || PK_PLATFORM_SIMD_VMX)

// Unicode text macro
// @TODO USE!!!!
#if !defined(TEXT)
#if PK_PLATFORM_TEXT_IS_CHAR16
#define _TEXT(x) u##x
#else
#define _TEXT(x) L##x
#endif
#define TEXT(x) _TEXT(x)
#endif

#undef PK_IS_PLATFORM_HEADER
