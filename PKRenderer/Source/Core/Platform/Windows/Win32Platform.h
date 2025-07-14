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

#endif