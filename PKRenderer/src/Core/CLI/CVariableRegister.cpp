#include "PrecompiledHeader.h"
#include "Core/Services/StringHashID.h"
#include "Core/CLI/Log.h"
#include "CVariableRegister.h"

namespace PK::Core::CLI
{
    using namespace PK::Utilities;
    using namespace PK::Core::Services;

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
        auto name = variable->CVarGetName();
        auto variableId = variable->variableId = StringHashID::StringToID(name);
        PK_THROW_ASSERT(m_variables.count(variableId) == 0, "CVar is already bound! (%s)", name);
        PK_LOG_VERBOSE("CVariableRegister.Bind: %s", name);
        m_variables[variableId] = variable;
    }

    void CVariableRegister::UnbindInstance(ICVariable* variable)
    {
        const auto& name = StringHashID::IDToString(variable->variableId);
        PK_THROW_ASSERT(variable->variableId != 0, "Cant unbind a unbound CVar! (%s)", name);
        m_variables.erase(variable->variableId);
        variable->variableId = 0u;
    }

    bool CVariableRegister::IsBoundInstance(const char* name) const
    {
        return m_variables.count(StringHashID::StringToID(name)) > 0;
    }

    void CVariableRegister::ExecuteInstance(const char** args, uint32_t count)
    {
        if (count > 0)
        {
            auto variableName = args[0];
            auto variableId = StringHashID::StringToID(variableName);
            --count;

            if (m_variables.count(variableId))
            {
                auto& variable = m_variables.at(variableId);

                if (count < variable->CVarGetMinArgs() || count > variable->CVarGetMaxArgs())
                {
                    variable->CVarInvalidArgCount();
                    return;
                }

                m_variables.at(variableId)->CVarExecute(args + 1, count);
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