#pragma once
#include "Math/Types.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/NativeInterface.h"
#include "Utilities/FenceRef.h"

namespace PK::Rendering::RHI
{
    struct WindowProperties
    {
        std::string title;
        std::string iconPath;
        uint32_t width;
        uint32_t height;
        bool vsync;
        bool cursorVisible;
    
        WindowProperties(const std::string& title = "PK Window", 
                            const std::string& iconPath = std::string(), 
                            uint32_t width = 1600u, 
                            uint32_t height = 912u, 
                            bool vsync = true, 
                            bool cursorVisible = true) :
            title(title), 
            iconPath(iconPath), 
            width(width), 
            height(height), 
            vsync(vsync), 
            cursorVisible(cursorVisible)
        {
        }
    };
    
    struct Window : public Utilities::NoCopy, public Utilities::NativeInterface<Window>
    {
        constexpr static uint32_t MIN_SIZE = 256u;

        static Utilities::Scope<Window> Create(const WindowProperties& properties);
        virtual ~Window() = default;
    
        virtual Math::uint3 GetResolution() const = 0;
        virtual Math::uint4 GetRect() const = 0;
        virtual float GetAspectRatio() const = 0;
        virtual bool IsAlive() const = 0;
        virtual bool IsMinimized() const = 0;
        virtual bool IsVSync() const = 0;
            
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void SetFrameFence(const Utilities::FenceRef& fence) = 0;
        virtual void SetCursorVisible(bool value) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual void PollEvents() const = 0;
        virtual void WaitEvents() const = 0;
        virtual void* GetNativeWindow() const = 0;

        std::function<void(int width, int height)> OnResize;
        std::function<void()> OnClose;
    };
}