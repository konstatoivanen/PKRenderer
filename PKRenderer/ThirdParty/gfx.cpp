#include "PrecompiledHeader.h"
#include "gfx.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif

// Was too lazy to do it myself
// Source: https://stackoverflow.com/questions/21421074/how-to-create-a-full-screen-window-on-the-current-monitor-with-glfw
static int mini(int x, int y)
{
    return x < y ? x : y;
}

static int maxi(int x, int y)
{
    return x > y ? x : y;
}

GLFWmonitor* PK_GLFW_GetCurrentMonitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; ++i) 
    {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap =
            maxi(0, mini(wx + ww, mx + mw) - maxi(wx, mx)) *
            maxi(0, mini(wy + wh, my + mh) - maxi(wy, my));

        if (bestoverlap < overlap) 
        {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}

void PK_GLFW_SetFullScreen(GLFWwindow* window, bool value, int32_t* inoutWindowedRect, const void** outNativeWindow)
{
    if (value)
    {
        auto monitor = PK_GLFW_GetCurrentMonitor(window);
        auto mode = glfwGetVideoMode(monitor);
        glfwGetWindowPos(window, inoutWindowedRect, inoutWindowedRect + 1);
        glfwGetWindowSize(window, inoutWindowedRect + 2, inoutWindowedRect + 3);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
#ifdef _WIN32
        // @TODO may not match the above fetcher and crash. :( 
        // @TODO deprecate GLFW
        * outNativeWindow = MonitorFromPoint({ inoutWindowedRect[0], inoutWindowedRect[1] }, MONITOR_DEFAULTTONEAREST);
#endif
    }
    else
    {
        glfwSetWindowMonitor(window, nullptr, inoutWindowedRect[0], inoutWindowedRect[1], inoutWindowedRect[2], inoutWindowedRect[3], GLFW_DONT_CARE);
    }
}

int PK_GLFW_CreateVkTemporarySurface(VkInstance instance, PK_GLFW_VkTemporarySurface& outSurface)
{
    // Create a temporary hidden window so that we can query & select a physical device with surface present capabilities.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    outSurface.window = glfwCreateWindow(1, 1, "Initialization Window", nullptr, nullptr);
    
    if (!outSurface.window)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return glfwCreateWindowSurface(instance, outSurface.window, nullptr, &outSurface.surface);
}

void PK_GLFW_DestroyVkTemporarySurface(VkInstance instance, PK_GLFW_VkTemporarySurface& outSurface)
{
    vkDestroySurfaceKHR(instance, outSurface.surface, nullptr);
    glfwDestroyWindow(outSurface.window);
}
