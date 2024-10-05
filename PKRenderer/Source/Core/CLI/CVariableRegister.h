#pragma once
#include <unordered_map>
#include <vector>
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FastMap.h"
#include "Core/CLI/CArguments.h"
#include "Core/CLI/CVariable.h"
#include "Core/ControlFlow/IStep.h"

namespace PK
{
    class CVariableRegister :
        public ISingleton<CVariableRegister>,
        public IStep<CArgumentsConst>,
        public IStep<CArgumentConst>
    {
        struct CVariableBinding
        {
            ICVariable* variable = nullptr;
            Unique<CArgumentsInlineDefault> arguments;
        };

    public:
        ~CVariableRegister();

        static void Bind(ICVariable* variable);
        static void Unbind(ICVariable* variable);
        static bool IsBound(const char* name);
        static void Execute(const char* const* args, uint32_t count);
        static void ExecuteParse(const char* arg);

        template<typename T>
        static void Create(const char* name, const T& value, const char* hint = "hint undefined", uint32_t minArgs = 0u)
        {
            if (Get() != nullptr)
            {
                auto variable = new CVariable<T>(name, value, hint, minArgs);
                Get()->m_scopes.push_back(Unique<ICVariable>(variable));
                Bind(variable);
            }
        }

        inline virtual void Step(CArgumentsConst arguments) final { ExecuteInstance(arguments.args, (uint32_t)arguments.count); }
        inline virtual void Step(CArgumentConst argument) final { ExecuteParseInstance(argument.arg); }

    private:
        void BindInstance(ICVariable* variable);
        void UnbindInstance(ICVariable* variable);
        bool IsBoundInstance(const char* name) const;
        void ExecuteInstance(const char* const* args, uint32_t count);
        void ExecuteInstance(const std::vector<std::string>& args);
        void ExecuteParseInstance(const char* arg);

        FastMap<NameID, CVariableBinding, Hash::TCastHash<NameID>> m_variables;
        std::vector<Unique<ICVariable>> m_scopes;
    };
}