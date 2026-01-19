#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "CVariableRegister.h"

namespace PK
{
    CVariableRegister::~CVariableRegister()
    {
        for (auto i = 0u; i < m_variables.GetCount(); ++i)
        {
            DestroyBinding(&m_variables[i].value);
        }
    }


    void CVariableRegister::Bind(ICVariable* variable)
    {
        if (Get() == nullptr)
        {
            PK_LOG_WARNING("CVariableRegister has not been initialized!");
            return;
        }

        Get()->BindInstance(variable, 0u);
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

        return Get()->IsBoundInstance(name);
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



    void CVariableRegister::DestroyBinding(CVariableBinding* binding)
    {
        if (binding->variable)
        {
            binding->variable->flags &= ~FLAG_IS_BOUND;
        }

        if (binding->variable && (binding->variable->flags & FLAG_IS_REGISTER_OWNED) != 0u)
        {
            binding->variable->~ICVariable();
            binding->variable = nullptr;
        }

        if (binding->arguments)
        {
            delete binding->arguments;
            binding->arguments = nullptr;
        }
    }

    void CVariableRegister::BindInstance(ICVariable* variable, uint32_t flags)
    {
        auto name = variable->name;
        auto index = 0u;
        auto isNew = m_variables.AddKey(name, &index);
        auto binding = &m_variables[index].value;
        PK_THROW_ASSERT(!isNew || !binding->variable, "CVar is already bound! (%s)", name.c_str());
        PK_LOG_VERBOSE_FUNC("%s", name.c_str());
        
        binding->variable = variable;
        binding->variable->flags = flags | FLAG_IS_BOUND;

        // Immediately call execute if there is one pending for this variable.
        if (!isNew && binding->arguments != nullptr)
        {
            ExecuteInstance(binding->arguments->arguments, binding->arguments->count);
            delete binding->arguments;
            binding->arguments = nullptr;
        }
    }

    void CVariableRegister::UnbindInstance(ICVariable* variable)
    {
        auto index = m_variables.GetIndex(variable->name);

        if (index != -1)
        {
            DestroyBinding(&m_variables[index].value);
            m_variables.RemoveAt(index);
        }
    }

    bool CVariableRegister::IsBoundInstance(const char* name) const
    {
        auto binding = m_variables.GetValuePtr(name);
        return binding && binding->variable != nullptr;
    }


    void CVariableRegister::ExecuteInstance(const char* const* args, uint32_t count)
    {
        if (count > 0)
        {
            auto index = 0u;
            auto isNew = m_variables.AddKey(NameID(args[0]), &index);
            auto binding = &m_variables[index].value;

            if (!isNew && binding->variable)
            {
                binding->variable->CVarExecute(args + 1, count - 1u);
            }
            else
            {
                // CVar was not found. cache arguments so that they can be executed upon binding.
                binding->variable = nullptr;
                binding->arguments = new CArgumentsInlineDefault(args, count);
            }
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
