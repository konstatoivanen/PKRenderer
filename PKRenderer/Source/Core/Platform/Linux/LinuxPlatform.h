#pragma once
#if PK_IS_PLATFORM_HEADER && PK_PLATFORM_LINUX

// @TODO add linux support.
#define PK_PLATFORM_X64 1
#define PK_PLATFORM_TEXT_IS_CHAR16 1
#define PK_PLATFORM_DEBUG_BREAK __asm__ volatile("int $0x03")
#define PK_PLAFTORM_DRIVER_TYPE LinuxDriver
#define VK_USE_PLATFORM_WAYLAND_KHR

PK_DECLARE_PLATFORM_INTERFACE(PlatformLinux, PlatformResourcesLinux)

#endif
