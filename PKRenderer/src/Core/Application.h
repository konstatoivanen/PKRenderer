#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/NoCopy.h"
#include "Core/Services/ServiceRegister.h"
#include "Core/Window.h"
#include "Rendering/GraphicsAPI.h"

int main(int argc, char** argv);

namespace PK::Core
{
    using namespace Utilities;
    using namespace Rendering;
    using namespace Services;

    struct ApplicationArguments
    {
        int count;
        char** args;
    };

    class Application : public NoCopy
    {
        public:
            Application(ApplicationArguments arguments, const std::string& name = "Application");
            virtual ~Application();
            void Close();
        
            inline static Application& Get() { return *s_Instance; }
            
            template<typename T>
            inline static T* GetService() { return Get().m_services->Get<T>(); }
    
            inline static Window* GetPrimaryWindow() { return Get().m_window.get(); }

        private:
            void Run();
    
        private:
            static Application* s_Instance;
            bool m_Running = true;
            
            Scope<GraphicsDriver> m_graphicsDriver;
            Scope<Window> m_window;
            Scope<ServiceRegister> m_services;
    
            friend int ::main(int argc, char** argv);
    };
}