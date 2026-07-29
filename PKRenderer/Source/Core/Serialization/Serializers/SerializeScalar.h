#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template<typename T>
    requires (!TIsPointer<T>) && TIsArithmetic<T>
    struct ISerializer<T, true>
    {
        static void ReadVal(SerialNodeRead node, T* rhs)
        {
            node >> *rhs;
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node << *rhs |= ryml::VAL_PLAIN;
        }
    };
}
#endif
