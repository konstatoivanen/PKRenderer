#pragma once
#include "Core/IApplication.h"

namespace PK::App
{
    struct RendererApplication : public IApplication
    {
        RendererApplication(const CArguments& arguments);
        ~RendererApplication();
        
        void Close() final;

        RHIDriver* GetRHIDriver() final { return m_graphicsDriver.get(); }
        Window* GetPrimaryWindow() final { return m_window.get(); }
        const RHIDriver* GetRHIDriver() const { return m_graphicsDriver.get(); }
        const Window* GetPrimaryWindow() const { return m_window.get(); }

    protected:
        void Execute() final;

    private:
        RHIDriverScope m_graphicsDriver;
        WindowScope m_window;

        uint32_t m_inactiveFrameInterval = 0u;
        bool m_isRunning = true;
    };
}