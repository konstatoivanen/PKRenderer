#pragma once
#include <cstdint>
#include <string>
#include "Utilities/NoCopy.h"
#include "Utilities/NameID.h"

namespace PK::Core::CLI
{
    struct ICVariable : public PK::Utilities::NoCopy
    {
        friend class CVariableRegister;

        ICVariable(const char* name) : name(name) {}

        virtual ~ICVariable() = 0 {};

        void CVarBind();
        void CVarUnbind();

        const char* CVarGetName() const { return name.c_str(); }

    protected:
        virtual void CVarExecute(const char** args, uint32_t count) = 0;
        virtual void CVarInvalidArgCount() const = 0;
        virtual uint32_t CVarGetMinArgs() const = 0;
        virtual uint32_t CVarGetMaxArgs() const = 0;

        Utilities::NameID name;
    };

    template<typename T>
    struct CVariable : public ICVariable
    {
        friend class CVariableRegister;

        CVariable(const char* name, const T& value, const char* hint = "cvar hint undefined.", uint32_t argsMin = 1u, uint32_t argsMax = 1u) :
            ICVariable(name),
            m_value(value),
            m_hint(hint),
            m_argsMin(argsMin),
            m_argsMax(argsMax)
        {
        };

        inline T& CVarGetValue() { return m_value; }

    protected:
        void CVarExecute(const char** args, uint32_t count) final;
        void CVarInvalidArgCount() const final;
        inline uint32_t CVarGetMinArgs() const final { return m_argsMin; }
        inline uint32_t CVarGetMaxArgs() const final { return m_argsMax; }

        T m_value;
        std::string m_hint;
        uint32_t m_argsMin;
        uint32_t m_argsMax;
    };

    typedef std::function<void(const char** args, uint32_t)> CVariableFunc;
}