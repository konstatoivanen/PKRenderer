#pragma once
#include "Utilities/NoCopy.h"
#include "Core/CLI/CArguments.h"
#include "Core/CLI/ILogger.h"
#include "Core/ServiceRegister.h"
#include "Graphics/GraphicsFwd.h"

int main(int argc, char** argv);

namespace PK::Core
{
    class Application : public Utilities::NoCopy
    {
    public:
        Application(CLI::CArguments arguments, const std::string& name = "Application");
        virtual ~Application();
        void Close();

        inline static Application& Get() { return *s_instance; }

        template<typename T>
        inline static T* GetService() { return Get().m_services->Get<T>(); }

        inline static Graphics::Window* GetPrimaryWindow() { return Get().m_window.get(); }

    private:
        void Execute();

    private:
        static Application* s_instance;
        bool m_isRunning = true;

        Utilities::Ref<CLI::ILogger> m_logger;
        Utilities::Scope<ServiceRegister> m_services;
        Graphics::DriverScope m_graphicsDriver;
        Graphics::WindowScope m_window;

        friend int ::main(int argc, char** argv);
    };
}