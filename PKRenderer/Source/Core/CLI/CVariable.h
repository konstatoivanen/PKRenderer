#pragma once
#include <cstdint>
#include <string>
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NameID.h"
#include "Core/Utilities/FixedString.h"

namespace PK
{
    struct ICVariable : public NoCopy
    {
        friend class CVariableRegister;

        ICVariable(const char* name) : name(name), flags(0u) {}

        virtual ~ICVariable() = 0;

        void CVarBind();
        void CVarUnbind();

    protected:
        virtual void CVarExecute(const char* const* args, uint32_t count) = 0;

        NameID name;
        uint32_t flags;
    };

    template<typename T>
    struct CVariable : public ICVariable
    {
        friend class CVariableRegister;

        CVariable(const char* name, const T& value, const char* hint = "cvar hint undefined.", uint32_t minArgs = 1u) :
            ICVariable(name),
            m_value(value),
            m_minArgs(minArgs),
            m_hint(hint)
        {
        };

        constexpr T& CVarGetValue() { return m_value; }
        constexpr const T& CVarGetValue() const { return m_value; }

    protected:
        void CVarExecute(const char* const* args, uint32_t count) final;

        T m_value;
        uint32_t m_minArgs;
        FixedString64 m_hint;
    };

    template<typename T>
    struct CVariableField : public ICVariable
    {
        T Value;

        CVariableField(const char* name, const T& initialValue = {}) : ICVariable(name), Value(initialValue) { this->CVarBind(); }
        ~CVariableField() { this->CVarUnbind(); }

        void CVarExecute(const char* const* args, uint32_t count) final;

        constexpr operator T&() { return Value; }
        constexpr operator const T&() const { return Value; }

        CVariableField& operator=(const T& value)
        {
            Value = value;
            return *this;
        }
    };

    typedef std::function<void(const char* const* args, uint32_t)> CVariableFunc;
    typedef std::function<void()> CVariableFuncSimple;
}
