#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "CVariableRegister.h"

namespace PK
{
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

    void CVariableRegister::Execute(const char* const* args, uint32_t count)
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
        auto iter = m_variables.find(name);
        PK_THROW_ASSERT(iter == m_variables.end() || iter->second.variable == nullptr, "CVar is already bound! (%s)", name.c_str());
        PK_LOG_VERBOSE("CVariableRegister.Bind: %s", name.c_str());

        // Immediately call execute if there is one pending for this variable.
        if (iter != m_variables.end() && iter->second.arguments != nullptr)
        {
            iter->second.variable = variable;
            ExecuteInstance(iter->second.arguments->arguments, iter->second.arguments->count);
            iter->second.arguments = nullptr;
            return;
        }

        m_variables[name].variable = variable;
    }

    void CVariableRegister::UnbindInstance(ICVariable* variable)
    {
        m_variables.erase(variable->name);
    }

    bool CVariableRegister::IsBoundInstance(const char* name) const
    {
        auto iter = m_variables.find(name);
        return iter != m_variables.end() && iter->second.variable != nullptr;
    }

    void CVariableRegister::ExecuteInstance(const char* const* args, uint32_t count)
    {
        if (count > 0)
        {
            auto name = NameID(args[0]);
            auto iter = m_variables.find(name);

            if (iter != m_variables.end() && iter->second.variable != nullptr)
            {
                auto variable = iter->second.variable;
                auto executeArgsCount = count - 1u;

                if (executeArgsCount < variable->CVarGetMinArgs())
                {
                    variable->CVarInvalidArgCount();
                    return;
                }

                variable->CVarExecute(args + 1, executeArgsCount);
                return;
            }

            // CVar was not found. cache arguments so that they can be executed upon binding.
            m_variables[name].arguments = CreateScope<CArgumentsInlineDefault>(args, count);
        }
    }

    void CVariableRegister::ExecuteInstance(const std::vector<std::string>& args)
    {
        std::vector<const char*> cstringArguments;

        for (auto& arg : args)
        {
            cstringArguments.push_back(arg.data());
        }

        ExecuteInstance(cstringArguments.data(), cstringArguments.size());
    }

    void CVariableRegister::ExecuteParseInstance(const char* arg)
    {
        CArgumentsInlineDefault args(arg, ' ');
        ExecuteInstance(args.arguments, args.count);
    }
}