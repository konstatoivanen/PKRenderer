#pragma once
#include <functional>
#include "Math/Types.h"
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics::RHI
{    
    struct RHIWindow : public Utilities::NoCopy, public Utilities::NativeInterface<RHIWindow>
    {
        constexpr static uint32_t MIN_SIZE = 256u;

        virtual ~RHIWindow() = 0;
    
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