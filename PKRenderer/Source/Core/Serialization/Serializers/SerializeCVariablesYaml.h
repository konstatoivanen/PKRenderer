#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    struct CVariablesYaml;
    template<> struct ISerializer<CVariablesYaml>{ static void ReadVal(const SerialNodeConst& node, CVariablesYaml* rhs);};
}
#endif
