#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    #define DECLARE_FIXEDSTRING_SERIALIZE(type)                                                                                                         \
    template<> bool Read<type>(const ConstNode& node, type* rhs) { auto substr = node.val(); *rhs = type(substr.len, substr.data()); return true; }     \
    template<> bool ReadKey<type>(const ConstNode& node, type* rhs) { auto substr = node.key(); *rhs = type(substr.len, substr.data()); return true; }  \
    template<> void Write<type>(Node& parent, const char* memberName, const type* rhs) { parent[memberName] << rhs->c_str() |= ryml::VAL_DQUO; }        \
    PK_YAML_DECLARE_READ_MEMBER(type)                                                                                                                   \

    DECLARE_FIXEDSTRING_SERIALIZE(FixedString16)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString32)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString64)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString128)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString256)
    DECLARE_FIXEDSTRING_SERIALIZE(FixedString512)

    #undef DECLARE_FIXEDSTRING_SERIALIZE
}
