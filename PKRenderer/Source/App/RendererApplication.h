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
        RHIWindow* GetPrimaryWindow() final { return m_window.get(); }
        const RHIDriver* GetRHIDriver() const { return m_graphicsDriver.get(); }
        const RHIWindow* GetPrimaryWindow() const { return m_window.get(); }

    protected:
        void Execute() final;

    private:
        RHIDriverScope m_graphicsDriver;
        RHIWindowScope m_window;

        bool m_isRunning = true;
    };
}