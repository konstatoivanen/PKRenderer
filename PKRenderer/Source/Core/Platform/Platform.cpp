#include "PrecompiledHeader.h"
#include "Core/Platform/Windows/Win32Driver.h"

namespace PK
{
    IPlatformWindowListener::~IPlatformWindowListener() = default;
    IPlatformWindowInputListener::~IPlatformWindowInputListener() = default;
    PlatformWindow::~PlatformWindow() = default;
    PlatformDriver::~PlatformDriver() = default;

    PlatformDriver* Platform::CreateDriver()
    { 
        auto nativeDriver = new PK_PLAFTORM_DRIVER_TYPE();
        return nativeDriver;
    }

    void Platform::DestroyDriver(PlatformDriver* driver)
    {
        auto nativeDriver = static_cast<PK_PLAFTORM_DRIVER_TYPE*>(driver);
        delete nativeDriver;
    }

    void Platform::PollEvents()
    {
        PlatformDriver::Get()->PollEvents();
    }

    void Platform::WaitEvents()
    {
        PlatformDriver::Get()->WaitEvents();
    }
    
    void* Platform::GetProcess()
    { 
        return PlatformDriver::Get()->GetProcess(); 
    }

    void* Platform::GetHelperWindow() 
    { 
        return PlatformDriver::Get()->GetHelperWindow(); 
    }

    void* Platform::LoadLibrary(const char* path) 
    { 
        return PlatformDriver::Get()->LoadLibrary(path); 
    }

    void Platform::FreeLibrary(void* handle) 
    { 
        PlatformDriver::Get()->FreeLibrary(handle); 
    }

    void* Platform::GetProcAddress(void* handle, const char* name) 
    { 
        return PlatformDriver::Get()->GetProcAddress(handle, name); 
    }

    bool Platform::GetHasFocus() 
    { 
        return PlatformDriver::Get()->GetHasFocus(); 
    }

    int2 Platform::GetDesktopSize() 
    { 
        return PlatformDriver::Get()->GetDesktopSize(); 
    }

    void* Platform::GetMonitorHandle(const int2& point, bool preferPrimary) 
    { 
        return PlatformDriver::Get()->GetMonitorHandle(point, preferPrimary); 
    }

    int4 Platform::GetMonitorRect(const int2& point, bool preferPrimary) 
    { 
        return PlatformDriver::Get()->GetMonitorRect(point, preferPrimary); 
    }

    PlatformWindow* Platform::CreateWindow(const PlatformWindowDescriptor& descriptor) 
    { 
        return PlatformDriver::Get()->CreateWindow(descriptor);
    }

    void Platform::DestroyWindow(PlatformWindow* window) 
    { 
        PlatformDriver::Get()->DestroyWindow(window);
    }

    void Platform::SetConsoleColor(uint32_t color) 
    { 
        PlatformDriver::Get()->SetConsoleColor(color); 
    }

    void Platform::SetConsoleVisible(bool value) 
    { 
        PlatformDriver::Get()->SetConsoleVisible(value); 
    }

    bool Platform::RemoteProcess(const char* executable, const char* arguments, std::string& outError) 
    { 
        return PlatformDriver::Get()->RemoteProcess(executable, arguments, outError); 
    }
}
