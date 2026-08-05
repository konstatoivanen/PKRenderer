#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Base/Containers/FixedString.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template<typename TChar, size_t capacity>
    struct ISerializer<IFixedString<TChar, capacity>>
    {
        using Type = IFixedString<TChar, capacity>;
        static void ReadVal(SerialNodeRead node, Type* rhs) { auto substr = node.val(); *rhs = Type(substr.len, substr.data()); }
        static void ReadKey(SerialNodeRead node, Type* rhs) { auto substr = node.key(); *rhs = Type(substr.len, substr.data()); }
        static void WriteVal(SerialNodeWrite node, const Type* rhs) { node << rhs->c_str() |= ryml::VAL_DQUO; }
        static void WriteKey(SerialNodeWrite node, const Type* rhs) { node.set_key(rhs->c_str()); }
    };
}
#endif
