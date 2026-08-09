#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template<typename T>
    requires (!TIsPointer<T>) && TIsArithmetic<T>
    struct ISerializer<T, void>
    {
        static void ReadVal(SerialNodeRead node, T* rhs)
        {
            node.load(rhs, false);
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.save(*rhs, ryml::VAL_PLAIN);
        }
    };
}
#endif
