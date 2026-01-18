#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/Ref.h"
#include "Core/CLI/CArguments.h"
#include "Core/CLI/ILogger.h"
#include "Core/ServiceRegister.h"
#include "Core/Rendering/RenderingFwd.h"

int main(int argc, char** argv);

namespace PK
{
    struct Window;

    extern struct IApplication* CreateProjectApplication(const CArguments& arguments);
    extern void FreeProjectApplication(IApplication* application);

    struct IApplication : public ISingleton<IApplication>
    {
        IApplication(const CArguments& arguments, const char* name, Ref<ILogger> logger);

        virtual ~IApplication() = 0;
        
        virtual void Close() = 0;
        
        virtual RHIDriver* GetRHIDriver() = 0;
        virtual Window* GetPrimaryWindow() = 0;
        virtual const RHIDriver* GetRHIDriver() const = 0;
        virtual const Window* GetPrimaryWindow() const = 0;

        template<typename T>
        T* GetService() { return m_services.Get<T>(); }

        inline Weak<ILogger> GetLogger() { return m_logger; }
        inline const Weak<ILogger> GetLogger() const { return m_logger; }

        constexpr const CArguments& GetArguments() const { return m_arguments; }
        constexpr const char* GetName() const { return m_name.c_str(); }
        constexpr const char* GetWorkingDirectory() const { return m_workingDirectory.c_str(); }

    protected:
        inline ServiceRegister* GetServices() { return &m_services; }

        virtual void Execute() = 0;
        
    private:
        const CArguments m_arguments;
        const FixedString256 m_name;
        const FixedString256 m_workingDirectory;

        Ref<ILogger> m_logger;
        ServiceRegister m_services;

        friend int ::main(int argc, char** argv);
    };
}