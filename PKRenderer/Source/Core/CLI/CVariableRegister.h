#pragma once
#include <unordered_map>
#include <vector>
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/ISingleton.h"
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
    public:
        static void Bind(ICVariable* variable);
        static void Unbind(ICVariable* variable);
        static bool IsBound(const char* name);
        static void Execute(const char** args, uint32_t count);
        static void ExecuteParse(const char* arg);

        template<typename T>
        static void Create(const char* name, const T& value, const char* hint = "hint undefined", uint32_t minArgs = 0u, uint32_t maxArgs = 0u)
        {
            if (Get() != nullptr)
            {
                auto variable = new CVariable<T>(name, value, hint, minArgs, maxArgs);
                Get()->m_scopes.push_back(Scope<ICVariable>(variable));
                Bind(variable);
            }
        }

        inline virtual void Step(CArgumentsConst arguments) final { ExecuteInstance(arguments.args, (uint32_t)arguments.count); }
        inline virtual void Step(CArgumentConst argument) final { ExecuteParseInstance(argument.arg); }

    private:
        void BindInstance(ICVariable* variable);
        void UnbindInstance(ICVariable* variable);
        bool IsBoundInstance(const char* name) const;
        void ExecuteInstance(const char** args, uint32_t count);
        void ExecuteParseInstance(const char* arg);

        std::unordered_map<NameID, ICVariable*> m_variables;
        std::vector<Scope<ICVariable>> m_scopes;
    };
}