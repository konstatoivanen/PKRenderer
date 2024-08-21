#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "CVariableRegister.h"

namespace PK
{
    CVariableRegister::~CVariableRegister()
    {
        for (auto i = 0u; i < m_variables.GetCount(); ++i)
        {
            m_variables.GetValueAt(i).arguments = nullptr;
        }
    }

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
        auto index = 0u;
        auto isNew = m_variables.AddKey(name, &index);
        auto reference = &m_variables.GetValueAt(index);
        PK_THROW_ASSERT(!isNew || reference->variable == nullptr, "CVar is already bound! (%s)", name.c_str());
        PK_LOG_VERBOSE("CVariableRegister.Bind: %s", name.c_str());

        // Immediately call execute if there is one pending for this variable.
        if (!isNew && reference->arguments != nullptr)
        {
            reference->variable = variable;
            ExecuteInstance(reference->arguments->arguments, reference->arguments->count);
            reference->arguments = nullptr;
            return;
        }

        reference->variable = variable;
    }

    void CVariableRegister::UnbindInstance(ICVariable* variable)
    {
        m_variables.Remove(variable->name);
    }

    bool CVariableRegister::IsBoundInstance(const char* name) const
    {
        auto reference = m_variables.GetValueRef(name);
        return reference && reference->variable != nullptr;
    }

    void CVariableRegister::ExecuteInstance(const char* const* args, uint32_t count)
    {
        if (count > 0)
        {
            auto name = NameID(args[0]);
            auto index = 0u;
            auto isNew = m_variables.AddKey(name, &index);
            auto reference = &m_variables.GetValueAt(index);

            if (!isNew)
            {
                auto variable = reference->variable;
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
            reference->variable = nullptr;
            reference->arguments = CreateScope<CArgumentsInlineDefault>(args, count);
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