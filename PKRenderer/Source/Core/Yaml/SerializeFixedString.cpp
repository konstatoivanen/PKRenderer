#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Yaml/Serialize.h"

namespace PK::YAML
{
    #define DECLARE_FIXEDSTRING_SERIALIZE(type)                                                                                             \
    template<> void Read<type>(const ConstNode& node, type* rhs) { auto substr = node.val(); *rhs = type(substr.len, substr.data());; }     \
    template<> void ReadKey<type>(const ConstNode& node, type* rhs) { auto substr = node.key(); *rhs = type(substr.len, substr.data()); }   \
    template<> void Write<type>(Node& node, const type* rhs) { node << rhs->c_str() |= ryml::VAL_DQUO; }                                    \

    DECLARE_FIXEDSTRING_SERIALIZE(FixedString16)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString32)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString64)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString128)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString256)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString512)

    #undef DECLARE_FIXEDSTRING_SERIALIZE
}
