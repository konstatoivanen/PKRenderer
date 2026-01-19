#pragma once
#include <vector>
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/FixedArena.h"
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
        constexpr static uint32_t FLAG_IS_REGISTER_OWNED = 1u << 0u;
        constexpr static uint32_t FLAG_IS_BOUND = 1u << 1u;

        struct CVariableBinding
        {
            ICVariable* variable = nullptr;
            CArgumentsInlineDefault* arguments = nullptr;
        };

    public:
        CVariableRegister() : m_variables(128, 2ull) {};
        ~CVariableRegister();

        static void Bind(ICVariable* variable);
        static void Unbind(ICVariable* variable);
        static bool IsBound(const char* name);

        static void Execute(const char* const* args, uint32_t count);
        static void ExecuteParse(const char* arg);

        template<typename T>
        static void Create(const char* name, const T& value, const char* hint = "hint undefined", uint32_t minArgs = 0u)
        {
            auto instance = Get();

            if (instance)
            {
                instance->BindInstance(instance->m_arena.New<CVariable<T>>(name, value, hint, minArgs), FLAG_IS_REGISTER_OWNED);
            }
        }

        inline virtual void Step(CArgumentsConst arguments) final { ExecuteInstance(arguments.args, (uint32_t)arguments.count); }
        inline virtual void Step(CArgumentConst argument) final { ExecuteParseInstance(argument.arg); }

    private:
        static void DestroyBinding(CVariableBinding* binding);
        void BindInstance(ICVariable* variable, uint32_t flags);
        void UnbindInstance(ICVariable* variable);
        bool IsBoundInstance(const char* name) const;

        void ExecuteInstance(const char* const* args, uint32_t count);
        void ExecuteInstance(const std::vector<std::string>& args);
        void ExecuteParseInstance(const char* arg);

        FastMap<NameID, CVariableBinding, Hash::TCastHash<NameID>> m_variables;
        FixedArena<32768ull> m_arena;
    };
}
