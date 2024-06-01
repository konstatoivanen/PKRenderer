#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/CLI/CArguments.h"
#include "Core/CLI/ILogger.h"
#include "Core/ServiceRegister.h"
#include "Core/RHI/RHI.h"

int main(int argc, char** argv);

namespace PK
{
    extern struct IApplication* CreateProjectApplication(const CArguments& arguments);

    struct IApplication : public ISingleton<IApplication>
    {
        IApplication(const CArguments& arguments, const std::string& name, Ref<ILogger> logger);

        virtual ~IApplication() = 0;
        
        virtual void Close() = 0;
        
        virtual RHIDriver* GetRHIDriver() = 0;
        virtual RHIWindow* GetPrimaryWindow() = 0;
        virtual const RHIDriver* GetRHIDriver() const = 0;
        virtual const RHIWindow* GetPrimaryWindow() const = 0;

        template<typename T>
        T* GetService() { return m_services->Get<T>(); }

        inline Weak<ILogger> GetLogger() { return m_logger; }
        inline const Weak<ILogger> GetLogger() const { return m_logger; }

        constexpr const CArguments& GetArguments() const { return m_arguments; }
        constexpr const std::string& GetName() const { return m_name; }
        constexpr const std::string& GetWorkingDirectory() const { return m_workingDirectory; }

    protected:
        inline ServiceRegister* GetServices() { return m_services.get(); }

        virtual void Execute() = 0;
        
    private:
        const CArguments m_arguments;
        const std::string m_name;
        const std::string m_workingDirectory;

        Ref<ILogger> m_logger;
        Scope<ServiceRegister> m_services;

        friend int ::main(int argc, char** argv);
    };
}