#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/NoCopy.h"
#include "Core/Services/ServiceRegister.h"
#include "Rendering/RHI/Driver.h"

int main(int argc, char** argv);

namespace PK::Core
{
    struct ApplicationArguments
    {
        int count;
        char** args;
    };

    class Application : public Utilities::NoCopy
    {
    public:
        Application(ApplicationArguments arguments, const std::string& name = "Application");
        virtual ~Application();
        void Close();

        inline static Application& Get() { return *s_Instance; }

        template<typename T>
        inline static T* GetService() { return Get().m_services->Get<T>(); }

        inline static Rendering::RHI::Window* GetPrimaryWindow() { return Get().m_window.get(); }

    private:
        void Execute();

    private:
        static Application* s_Instance;
        bool m_Running = true;

        Utilities::Scope<Rendering::RHI::Driver> m_graphicsDriver;
        Utilities::Scope<Rendering::RHI::Window> m_window;
        Utilities::Scope<Services::ServiceRegister> m_services;

        friend int ::main(int argc, char** argv);
    };
}