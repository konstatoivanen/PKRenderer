#pragma once
#if PK_IS_PLATFORM_HEADER && PK_PLATFORM_WINDOWS

#if defined(_M_X64)
#define PK_PLATFORM_X64 1
#endif
#if defined(_M_ARM64)
#define PK_PLATFORM_ARM64 1
#endif
#define PK_PLATFORM_DEBUG_BREAK __debugbreak()
#define PK_PLAFTORM_DRIVER_TYPE Win32Driver
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

namespace PK::Platform
{
    inline static uint32_t InterlockedExchange(volatile uint32_t* dst, uint32_t exchange) { return _InterlockedExchange(dst, exchange); }
    inline static uint32_t InterlockedCompareExchange(volatile uint32_t* dst, uint32_t exchange, uint32_t comperand) { return _InterlockedCompareExchange(dst, exchange, comperand); }
    inline static uint32_t InterlockedAdd(volatile uint32_t* dst, uint32_t value) { return _InterlockedExchangeAdd(dst, value); }
    inline static uint32_t InterlockedIncrement(volatile uint32_t* dst) { return _InterlockedIncrement(dst); }
    inline static uint32_t InterlockedDecrement(volatile uint32_t* dst) { return _InterlockedDecrement(dst); }
    inline static uint32_t AtomicRead(const volatile uint32_t* dst) { return (unsigned)__iso_volatile_load32(reinterpret_cast<const volatile int32_t*>(dst)); }
    inline static void AtomicStore(uint32_t volatile* dst, uint32_t value) { __iso_volatile_store32(reinterpret_cast<volatile int32_t*>(dst), value); }
    inline static uint64_t BitScan64(uint64_t mask) { auto index = 0ul; return _BitScanForward64(&index, mask) ? index : 64u; }
}

#endif
