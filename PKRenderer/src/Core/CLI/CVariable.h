#pragma once
#include "Utilities/NoCopy.h"
#include <cstdint>

namespace PK::Core::CLI
{
    struct ICVariable : public PK::Utilities::NoCopy
    {
        friend class CVariableRegister;

        virtual ~ICVariable() = 0 {};

        void CVarBind();
        void CVarUnbind();

    protected:
        virtual void CVarExecute(const char** args, uint32_t count) = 0;
        virtual void CVarInvalidArgCount() const = 0;
        virtual const char* CVarGetName() const = 0;
        virtual uint32_t CVarGetMinArgs() const = 0;
        virtual uint32_t CVarGetMaxArgs() const = 0;

        uint32_t variableId = 0;
    };

    template<typename T>
    struct CVariable : public ICVariable
    {
        friend class CVariableRegister;

        CVariable(const char* name, const T& value, const char* hint = "cvar hint undefined.", uint32_t argsMin = 1u, uint32_t argsMax = 1u) :
            m_value(value),
            m_name(name),
            m_hint(hint),
            m_argsMin(argsMin),
            m_argsMax(argsMax)
        {
        };

        inline T& CVarGetValue() { return m_value; }

    protected: 
        void CVarExecute(const char** args, uint32_t count) final;
        void CVarInvalidArgCount() const final;
        inline const char* CVarGetName() const final { return m_name.c_str(); }
        inline uint32_t CVarGetMinArgs() const final { return m_argsMin; }
        inline uint32_t CVarGetMaxArgs() const final { return m_argsMax; }

        T m_value;
        std::string m_name;
        std::string m_hint;
        uint32_t m_argsMin;
        uint32_t m_argsMax;
    };

    typedef std::function<void(const char** args, uint32_t)> CVariableFunc;
}