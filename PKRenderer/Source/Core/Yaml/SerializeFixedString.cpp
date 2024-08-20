#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include <rapidyaml/ryaml.h>
#include "Core/Yaml/RapidyamlFwd.h"

namespace PK::YAML
{
    #define DECLARE_FIXEDSTRING_READ(type)                                                                                                                  \
    template<> bool Read<type>(const ConstNode& node, type* rhs) { auto substr = node.val(); *rhs = type(substr.len, substr.data()); return true; }         \
    template<> bool ReadKey<type>(const ConstNode& node, type* rhs) { auto substr = node.key(); *rhs = type(substr.len, substr.data()); return true; }      \
    PK_YAML_DECLARE_READ_MEMBER(type)                                                                                                                       \

    DECLARE_FIXEDSTRING_READ(FixedString16)
    DECLARE_FIXEDSTRING_READ(FixedString32)
    DECLARE_FIXEDSTRING_READ(FixedString64)
    DECLARE_FIXEDSTRING_READ(FixedString128)
    DECLARE_FIXEDSTRING_READ(FixedString256)
    DECLARE_FIXEDSTRING_READ(FixedString512)

    #undef DECLARE_FIXEDSTRING_READ
}
