#include "PrecompiledHeader.h"
#include "Core/Utilities/Parsing.h"
#include "Core/Math/Math.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "CVariable.h"

namespace PK
{
    ICVariable::~ICVariable() = default;

    void ICVariable::CVarBind() { CVariableRegister::Bind(this); }
    void ICVariable::CVarUnbind() { CVariableRegister::Unbind(this); }

    template<> void CVariable<CVariableFunc>::CVarExecute(const char* const* args, uint32_t count) { m_value(args, count); }
    template<> void CVariable<CVariableFunc>::CVarInvalidArgCount() const { PK_LOG_INFO("% //%s", CVarGetName(), m_hint.c_str()); }

    template<> void CVariable<CVariableFuncSimple>::CVarExecute([[maybe_unused]] const char* const* args, [[maybe_unused]] uint32_t count) { m_value(); }
    template<> void CVariable<CVariableFuncSimple>::CVarInvalidArgCount() const { PK_LOG_INFO("% //%s", CVarGetName(), m_hint.c_str()); }

    #define PK_DECLARE_CVAR_SPECIALIZATION(TFormat, TType) \
    template<> void CVariable<TType>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count) { m_value = Parse::FromString<TType>(args[0]); PK_LOG_INFO("%s = %"#TFormat, name.c_str(), m_value); } \
    template<> void CVariable<TType>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %"#TFormat" // %s", name.c_str(), m_value, m_hint.c_str()); } \
    template<> void CVariable<TType*>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count) { *m_value = Parse::FromString<TType>(args[0]); PK_LOG_INFO("%s = %"#TFormat, name.c_str(), *m_value); } \
    template<> void CVariable<TType*>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %"#TFormat" // %s", name.c_str(), *m_value, m_hint.c_str()); } \
    template<> void CVariableField<TType>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count) { Value = Parse::FromString<TType>(args[0]); PK_LOG_INFO("%s = %"#TFormat, name.c_str(), Value); } \
    template<> void CVariableField<TType>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %"#TFormat, name.c_str()); } \
    template<> uint32_t CVariableField<TType>::CVarGetMinArgs() const { return 1u; } \

    #define PK_DECLARE_CVAR_VECTOR_SPECIALIZATION(TType, TCount) \
    template<> void CVariable<TType>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count) { Parse::StringsToArray(args, &m_value.x, TCount); PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&m_value.x, TCount).c_str());} \
    template<> void CVariable<TType>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %s // %s", name.c_str(), Parse::ArrayToString(&m_value.x, TCount).c_str(), m_hint.c_str()); } \
    template<> void CVariable<TType*>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count) { Parse::StringsToArray(args, &m_value->x, TCount); PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&m_value->x, TCount).c_str()); } \
    template<> void CVariable<TType*>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %s // %s", name.c_str(), Parse::ArrayToString(&m_value->x, TCount).c_str(), m_hint.c_str()); } \
    template<> void CVariableField<TType>::CVarExecute(const char* const* args, [[maybe_unused]] uint32_t count) { Parse::StringsToArray(args, &Value.x, TCount); PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&Value.x, TCount).c_str()); } \
    template<> void CVariableField<TType>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %s", name.c_str(), Parse::ArrayToString(&Value.x, TCount).c_str()); } \
    template<> uint32_t CVariableField<TType>::CVarGetMinArgs() const { return TCount; } \

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
}
