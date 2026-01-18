#include "PrecompiledHeader.h"
#include "Core/Utilities/Parse.h"
#include "Core/Math/Math.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "CVariable.h"

namespace PK
{
    ICVariable::~ICVariable() = default;

    void ICVariable::CVarBind() { CVariableRegister::Bind(this); }
    void ICVariable::CVarUnbind() { CVariableRegister::Unbind(this); }

    template<> void CVariable<CVariableFunc>::CVarExecute(const char* const* args, uint32_t count) 
    {
        if (m_minArgs > count)
        {
            PK_LOG_INFO("% //%s", name.c_str(), m_hint.c_str());
        }
        else
        {
            m_value(args, count); 
        }
    }

    template<> void CVariable<CVariableFuncSimple>::CVarExecute([[maybe_unused]] const char* const* args, [[maybe_unused]] uint32_t count) { m_value(); }

    #define PK_CVAR_CHECK_ARG_COUNT(count, min, ...) if (count < min) { PK_LOG_INFO(__VA_ARGS__); return; }

    #define PK_DECLARE_CVAR_SPECIALIZATION(TFormat, TType) \
    template<> void CVariable<TType>::CVarExecute(const char* const* args, uint32_t count)\
    {\
        PK_CVAR_CHECK_ARG_COUNT(count, 1u, "%s = %"#TFormat" // %s", name.c_str(), m_value, m_hint.c_str())\
        m_value = Parse::FromString<TType>(args[0]); PK_LOG_INFO("%s = %"#TFormat, name.c_str(), m_value);\
    }\
    template<> void CVariable<TType*>::CVarExecute(const char* const* args, uint32_t count)\
    {\
        PK_CVAR_CHECK_ARG_COUNT(count, 1u, "%s = %"#TFormat" // %s", name.c_str(), *m_value, m_hint.c_str())\
        *m_value = Parse::FromString<TType>(args[0]); PK_LOG_INFO("%s = %"#TFormat, name.c_str(), *m_value);\
    }\
    template<> void CVariableField<TType>::CVarExecute(const char* const* args, uint32_t count)\
    {\
        PK_CVAR_CHECK_ARG_COUNT(count, 1u, "%s = %"#TFormat, name.c_str())\
        Value = Parse::FromString<TType>(args[0]); PK_LOG_INFO("%s = %"#TFormat, name.c_str(), Value);\
    }\
    \

    #define PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(TType, TCount)\
    template<> void CVariable<TType>::CVarExecute(const char* const* args, uint32_t count)\
    {\
        PK_CVAR_CHECK_ARG_COUNT(count, TCount, "%s = %s // %s", name.c_str(), Parse::ArrayToString(&m_value.x, TCount).c_str(), m_hint.c_str())\
        Parse::StringsToArray(args, &m_value.x, TCount); PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&m_value.x, TCount).c_str());\
    }\
    template<> void CVariable<TType*>::CVarExecute(const char* const* args, uint32_t count)\
    {\
        PK_CVAR_CHECK_ARG_COUNT(count, TCount, "%s = %s // %s", name.c_str(), Parse::ArrayToString(&m_value->x, TCount).c_str(), m_hint.c_str())\
        Parse::StringsToArray(args, &m_value->x, TCount); PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&m_value->x, TCount).c_str());\
    }\
    template<> void CVariableField<TType>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count)\
    {\
        PK_CVAR_CHECK_ARG_COUNT(count, TCount, "%s = %s", name.c_str(), Parse::ArrayToString(&Value.x, TCount).c_str())\
        Parse::StringsToArray(args, &Value.x, TCount); PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&Value.x, TCount).c_str());\
    }\
    \

    PK_DECLARE_CVAR_SPECIALIZATION(u, uint8_t)
    PK_DECLARE_CVAR_SPECIALIZATION(i, int8_t)
    PK_DECLARE_CVAR_SPECIALIZATION(u, uint16_t)
    PK_DECLARE_CVAR_SPECIALIZATION(i, int16_t)
    PK_DECLARE_CVAR_SPECIALIZATION(u, uint32_t)
    PK_DECLARE_CVAR_SPECIALIZATION(i, int32_t)
    PK_DECLARE_CVAR_SPECIALIZATION(ull, uint64_t)
    PK_DECLARE_CVAR_SPECIALIZATION(ill, int64_t)
    PK_DECLARE_CVAR_SPECIALIZATION(f, float)
    PK_DECLARE_CVAR_SPECIALIZATION(i, bool)

    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(float2, 2)
    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(float3, 3)
    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(float4, 4)

    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(uint2, 2)
    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(uint3, 3)
    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(uint4, 4)

    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(int2, 2)
    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(int3, 3)
    PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(int4, 4)

    #undef PK_DECLARE_CVAR_SPECIALIZATION
    #undef PK_DECLARE_CVAR_VECTOR_SPECIALIZATION
    #undef PK_CVAR_CHECK_ARG_COUNT
}
