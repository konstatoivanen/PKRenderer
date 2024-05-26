#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/CLI/CArguments.h"
#include "Core/CLI/ILogger.h"
#include "Core/ServiceRegister.h"
#include "Core/RHI/RHI.h"

int main(int argc, char** argv);

namespace PK::App
{
    class Application : public NoCopy
    {
    public:
        Application(CArguments arguments, const std::string& name = "Application");
        virtual ~Application();
        void Close();

        inline static Application& Get() { return *s_instance; }

        template<typename T>
        inline static T* GetService() { return Get().m_services->Get<T>(); }

        inline static RHIWindow* GetPrimaryWindow() { return Get().m_window.get(); }

    private:
        void Execute();

    private:
        static Application* s_instance;
        bool m_isRunning = true;

        Ref<ILogger> m_logger;
        Scope<ServiceRegister> m_services;
        RHIDriverScope m_graphicsDriver;
        RHIWindowScope m_window;

        friend int ::main(int argc, char** argv);
    };
}