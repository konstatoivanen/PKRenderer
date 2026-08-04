#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"

// @TODO Temp
#include "Core/RHI/Structs.h"

namespace PK
{
    template<typename T>
    requires TIsEnum<T> && (!TIsFlagsEnum<T>)
    struct ISerializer<T, true>
    {
        static void ReadVal(SerialNodeRead node, T* rhs)
        {
            auto substr = node.val();
            FixedString128 value(substr.len, substr.data());
            
            // @TODO temp guard against elementype as we need to sort out the enum names correctly for that one.
            if constexpr (TIsSame<T, ElementType>)
            {
                *rhs = RHIEnumConvert::StringToElementType(value);
            }
            else
            {
                *rhs = TReflectEnum<T>::FromString(value);
            }
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            if constexpr (TIsSame<T, ElementType>)
            {
                node << RHIEnumConvert::ElementTypeToString(*rhs) |= ryml::VAL_DQUO;
            }
            else
            {
                node << TReflectEnum<T>::ToString(*rhs) |= ryml::VAL_DQUO;
            }
        }
    };

    template<typename T> 
    requires TIsEnum<T> && TIsFlagsEnum<T>
    struct ISerializer<T, true>
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
            node << value |= ryml::VAL_DQUO;
        }
    };
}
#endif
