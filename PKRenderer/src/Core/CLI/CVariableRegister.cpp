#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "CVariableRegister.h"

namespace PK::Core::CLI
{
    using namespace PK::Utilities;

    void CVariableRegister::Bind(ICVariable* variable)
    {
        if (Get() == nullptr)
        {
            PK_LOG_WARNING("CVariableRegister has not been initialized!");
            return;
        }

        Get()->BindInstance(variable);
    }

    void CVariableRegister::Unbind(ICVariable* variable)
    {
        if (Get() == nullptr)
        {
            PK_LOG_WARNING("CVariableRegister has not been initialized!");
            return;
        }

        Get()->UnbindInstance(variable);
    }

    bool CVariableRegister::IsBound(const char* name)
    {
        if (Get() == nullptr)
        {
            PK_LOG_WARNING("CVariableRegister has not been initialized!");
            return false;
        }

        return Get()->IsBound(name);
    }

    void CVariableRegister::Execute(const char** args, uint32_t count)
    {
        if (Get() == nullptr)
        {
            PK_LOG_WARNING("CVariableRegister has not been initialized!");
            return;
        }

        Get()->ExecuteInstance(args, count);
    }

    void CVariableRegister::ExecuteParse(const char* arg)
    {
        if (Get() == nullptr)
        {
            PK_LOG_WARNING("CVariableRegister has not been initialized!");
            return;
        }

        Get()->ExecuteParseInstance(arg);
    }

    void CVariableRegister::BindInstance(ICVariable* variable)
    {
        auto name = variable->name;
        PK_THROW_ASSERT(m_variables.count(name) == 0, "CVar is already bound! (%s)", name.c_str());
        PK_LOG_VERBOSE("CVariableRegister.Bind: %s", name.c_str());
        m_variables[name] = variable;
    }

    void CVariableRegister::UnbindInstance(ICVariable* variable)
    {
        auto name = variable->name;
        m_variables.erase(name);
    }

    bool CVariableRegister::IsBoundInstance(const char* name) const
    {
        return m_variables.count(NameID(name)) > 0;
    }

    void CVariableRegister::ExecuteInstance(const char** args, uint32_t count)
    {
        if (count > 0)
        {
            auto name = NameID(args[0]);
            --count;

            auto iter = m_variables.find(name);

            if (iter != m_variables.end())
            {
                auto& variable = iter->second;

                if (count < variable->CVarGetMinArgs() || count > variable->CVarGetMaxArgs())
                {
                    variable->CVarInvalidArgCount();
                    return;
                }

                variable->CVarExecute(args + 1, count);
            }
        }
    }

    void CVariableRegister::ExecuteParseInstance(const char* arg)
    {
        std::istringstream stream(arg);
        std::string element;

        std::vector<std::string> stringArguments;
        std::vector<const char*> cstringArguments;

        while (getline(stream, element, ' '))
        {
            stringArguments.push_back(element);
            cstringArguments.push_back(stringArguments.back().c_str());
        }

        ExecuteInstance(cstringArguments.data(), (uint32_t)cstringArguments.size());
    }
}