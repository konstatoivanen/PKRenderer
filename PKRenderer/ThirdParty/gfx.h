#pragma once
#ifdef _WIN32
// Exposes full screen ext
#define NOMINMAX 
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
// @TODO DEPRECATE GLFW
// Replace with custom platform specific windowing apis.
// Resolves issues with input, vulkan dependencies and window initialization order dependencies.
#include <GLFW/glfw3.h>

// Returns monitor that the window currently resides on
GLFWmonitor* PK_GLFW_GetCurrentMonitor(GLFWwindow* window);
void PK_GLFW_SetFullScreen(GLFWwindow* window, bool value, int32_t* inoutWindowedRect, const void** outNativeMonitor);
