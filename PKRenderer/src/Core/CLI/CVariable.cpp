#include "PrecompiledHeader.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/CLI/Log.h"
#include "CVariable.h"

namespace PK::Core::CLI
{
    void ICVariable::CVarBind() { CVariableRegister::Bind(this); }
    void ICVariable::CVarUnbind() { CVariableRegister::Unbind(this); }
    
    template<> void CVariable<CVariableFunc>::CVarExecute(const char** args, uint32_t count) { m_value(args, count); }
    template<> void CVariable<CVariableFunc>::CVarInvalidArgCount() const { PK_LOG_INFO("% //%s", CVarGetName(), m_hint.c_str()); }

    #define PK_DECLARE_CVAR_SPECIALIZATION(Format, CastFormat, ParseFormat) \
    template<> void CVariable<CastFormat>::CVarExecute(const char** args, uint32_t count) { m_value = (CastFormat)ato##ParseFormat(args[0]); PK_LOG_INFO("%s = %"#Format, m_name.c_str(), m_value); } \
    template<> void CVariable<CastFormat>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %"#Format" // %s", m_name.c_str(), m_value, m_hint.c_str()); } \
    template<> void CVariable<CastFormat*>::CVarExecute(const char** args, uint32_t count) { *m_value = (CastFormat)ato##ParseFormat(args[0]); PK_LOG_INFO("%s = %"#Format, m_name.c_str(), *m_value); } \
    template<> void CVariable<CastFormat*>::CVarInvalidArgCount() const { PK_LOG_INFO("%s = %"#Format" // %s", m_name.c_str(), *m_value, m_hint.c_str()); } \

    PK_DECLARE_CVAR_SPECIALIZATION(u,   uint8_t,   i)
    PK_DECLARE_CVAR_SPECIALIZATION(i,   int8_t,    i)
    PK_DECLARE_CVAR_SPECIALIZATION(u,   uint16_t,  i)
    PK_DECLARE_CVAR_SPECIALIZATION(i,   int16_t,   i)
    PK_DECLARE_CVAR_SPECIALIZATION(u,   uint32_t,  i)
    PK_DECLARE_CVAR_SPECIALIZATION(i,   int32_t,   i)
    PK_DECLARE_CVAR_SPECIALIZATION(ull, uint64_t,  ll)
    PK_DECLARE_CVAR_SPECIALIZATION(ill, int64_t,   ll)
    PK_DECLARE_CVAR_SPECIALIZATION(f,   float,     f)
    PK_DECLARE_CVAR_SPECIALIZATION(i,   bool,      i)

    #undef PK_DECLARE_CVAR_SPECIALIZATION
}
