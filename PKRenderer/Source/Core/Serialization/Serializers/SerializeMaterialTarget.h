#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    struct MaterialTarget;

    template<> struct ISerializer<MaterialTarget>
    {
        static void ReadVal(SerialNodeRead node, MaterialTarget* rhs);
        static void WriteVal(SerialNodeWrite node, MaterialTarget const* rhs);
    };
}
#endif
