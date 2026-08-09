#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template<typename T>
    requires TIsEnum<T> && (!TIsFlagsEnum<T>)
    struct ISerializer<T, void>
    {
        static void ReadVal(SerialNodeRead node, T* rhs)
        {
            auto substr = node.val();
            FixedString128 value(substr.len, substr.data());
            *rhs = TReflectEnum<T>::FromString(value);
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.save(TReflectEnum<T>::ToString(*rhs), ryml::VAL_PLAIN);
        }
    };

    template<typename T> 
    requires TIsEnum<T> && TIsFlagsEnum<T>
    struct ISerializer<T, void>
    {
        static void ReadVal(SerialNodeRead node, T* rhs)
        {
            auto substr = node.val();
            FixedString256 value(substr.len, substr.data());
            *rhs = TReflectEnum<T>::FlagsFromString(value);
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            char value[256]{};
            TReflectEnum<T>::FlagsToString(*rhs, value, 256);
            node.save(value, ryml::VAL_PLAIN);
        }
    };
}
#endif
