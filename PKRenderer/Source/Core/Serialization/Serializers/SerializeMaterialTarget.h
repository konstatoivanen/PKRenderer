#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    struct MaterialTarget;

    template<> struct ISerializer<MaterialTarget>
    {
        static void ReadVal(const SerialNodeConst& node, MaterialTarget* rhs);
        static void WriteVal(SerialNode& node, MaterialTarget const* rhs);
    };
}
#endif
