#pragma once
// Some manual guarding to avoid user error
#define PK_IS_PLATFORM_HEADER 1
#define _CRT_INTERNAL_NONSTDC_NAMES 1

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

#include "Core/Math/MathFwd.h"

namespace PK
{
    struct IPlatform
    {
        IPlatform(IPlatform const&) = delete;
        IPlatform& operator=(IPlatform const&) = delete;
        IPlatform() = default;

        static void FatalExit(const char* message);
        static void PollEvents();
        static void WaitEvents();

        // For objects that need to be cleaned in case of a crash
        // Do not use for sub allocations of other managed objects.
        static void AddManagedAllocation(void* object, void (*destructor)(void*));
        static void ManagedDeallocate(void* ptr);

        static int Initialize() = delete;
        static int Terminate() = delete;

        static void* AllocateAligned(size_t size, size_t alignment) = delete;
        static void FreeAligned(void* block) = delete;
        static struct PlatformMemoryInfo GetMemoryInfo() = delete;

        static void PollEvents(bool wait) = delete;
        static void* GetProcess() = delete;
        static void* GetHelperWindow() = delete;

        static void* LoadLibrary(const char* path) = delete;
        static void FreeLibrary(void* handle) = delete;
        static void* GetProcAddress(void* handle, const char* name) = delete;

        static double GetTimeSeconds() = delete;
        static uint64_t GetTimeCycles() = delete;

        static bool GetHasFocus() = delete;
        static int2 GetDesktopSize() = delete;
        static int4 GetMonitorRect(const int2& point, bool preferPrimary) = delete;
        static void* GetNativeMonitorHandle(const int2& point, bool preferPrimary) = delete;

        static struct PlatformWindow* CreateWindow(const struct PlatformWindowDescriptor& descriptor) = delete;
        static void DestroyWindow(struct PlatformWindow* window) = delete;

        static void SetInputHandler(struct InputHandler* handler) = delete;

        static const char* GetClipboardString() = delete;
        static void SetClipboardString(const char* str) = delete;

        static void SetConsoleColor(uint32_t color) = delete;
        static void SetConsoleVisible(bool value) = delete;
        static uint32_t RemoteProcess(const char* executable, const char* arguments) = delete;

        static uint32_t InterlockedExchange(volatile uint32_t* dst, uint32_t exchange) = delete;
        static uint32_t InterlockedCompareExchange(volatile uint32_t* dst, uint32_t exchange, uint32_t comperand) = delete;
        static uint32_t InterlockedAdd(volatile uint32_t* dst, uint32_t value) = delete;
        static uint32_t InterlockedIncrement(volatile uint32_t* dst) = delete;
        static uint32_t InterlockedDecrement(volatile uint32_t* dst) = delete;
        static uint32_t AtomicRead(const volatile uint32_t* dst) = delete;
        static void AtomicStore(volatile uint32_t* dst, uint32_t value) = delete;
        static uint64_t BitScan64(uint64_t mask) = delete;
    };

#if PK_PLATFORM_WINDOWS
    struct Win32Platform;
    typedef Win32Platform Platform;
#elif PK_PLATFORM_LINUX
    struct LinuxPlatform;
    typedef LinuxPlatform Platform;
#else
    struct IPlatform;
    typedef IPlatform Platform;
#endif
}

// Bypass platform alloc. 
// Visual studio doesnt show object types in memory profiler if they're not immediately cast to the correct type.
#if 1 
#define PK_SYSTEM_DEFAULT_ALIGN 16
#define PK_SYSTEM_ALIGNED_ALLOC(size, align) PK::Platform::AllocateAligned(size, align)
#define PK_SYSTEM_ALIGNED_FREE(ptr) PK::Platform::FreeAligned(ptr)
#define PK_SYSTEM_ERROR(message) PK::Platform::FatalExit(message)
#endif

#include "Windows/Win32Platform.h"
#include "Linux/LinuxPlatform.h"
#include "Core/Utilities/Memory.h"

#ifndef PK_PLATFORM_X64
#define PK_PLATFORM_X64 0
#endif

#ifndef PK_PLATFORM_ARM64
#define PK_PLATFORM_ARM64 0
#endif

#ifndef PK_PLATFORM_DEBUG_BREAK
#define PK_PLATFORM_DEBUG_BREAK
#endif

#ifndef PK_PLATFORM_TEXT_IS_CHAR16
#define PK_PLATFORM_TEXT_IS_CHAR16 0
#endif

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
