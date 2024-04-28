#pragma once
#include <yaml-cpp/yaml.h>
#include "Math/Types.h"

namespace YAML
{
    #define PK_DECLARE_YAML_CONVERT(type)               \
    template<>                                          \
    struct convert<type>                                \
    {                                                   \
        static Node encode(const type& rhs);            \
        static bool decode(const Node& node, type& rhs);\
    };                                                  \

    PK_DECLARE_YAML_CONVERT(PK::Math::float2)
    PK_DECLARE_YAML_CONVERT(PK::Math::float3)
    PK_DECLARE_YAML_CONVERT(PK::Math::float4)

    PK_DECLARE_YAML_CONVERT(PK::Math::double2)
    PK_DECLARE_YAML_CONVERT(PK::Math::double3)
    PK_DECLARE_YAML_CONVERT(PK::Math::double4)

    PK_DECLARE_YAML_CONVERT(PK::Math::float2x2)
    PK_DECLARE_YAML_CONVERT(PK::Math::float3x3)
    PK_DECLARE_YAML_CONVERT(PK::Math::float4x4)
    PK_DECLARE_YAML_CONVERT(PK::Math::float3x4)

    PK_DECLARE_YAML_CONVERT(PK::Math::double2x2)
    PK_DECLARE_YAML_CONVERT(PK::Math::double3x3)
    PK_DECLARE_YAML_CONVERT(PK::Math::double4x4)

    PK_DECLARE_YAML_CONVERT(PK::Math::short2)
    PK_DECLARE_YAML_CONVERT(PK::Math::short3)
    PK_DECLARE_YAML_CONVERT(PK::Math::short4)

    PK_DECLARE_YAML_CONVERT(PK::Math::ushort2)
    PK_DECLARE_YAML_CONVERT(PK::Math::ushort3)
    PK_DECLARE_YAML_CONVERT(PK::Math::ushort4)

    PK_DECLARE_YAML_CONVERT(PK::Math::ushort2x2)
    PK_DECLARE_YAML_CONVERT(PK::Math::ushort3x3)
    PK_DECLARE_YAML_CONVERT(PK::Math::ushort4x4)

    PK_DECLARE_YAML_CONVERT(PK::Math::sbyte4)
    PK_DECLARE_YAML_CONVERT(PK::Math::byte4)

    PK_DECLARE_YAML_CONVERT(PK::Math::int2)
    PK_DECLARE_YAML_CONVERT(PK::Math::int3)
    PK_DECLARE_YAML_CONVERT(PK::Math::int4)

    PK_DECLARE_YAML_CONVERT(PK::Math::uint2)
    PK_DECLARE_YAML_CONVERT(PK::Math::uint3)
    PK_DECLARE_YAML_CONVERT(PK::Math::uint4)

    PK_DECLARE_YAML_CONVERT(PK::Math::long2)
    PK_DECLARE_YAML_CONVERT(PK::Math::long3)
    PK_DECLARE_YAML_CONVERT(PK::Math::long4)

    PK_DECLARE_YAML_CONVERT(PK::Math::ulong2)
    PK_DECLARE_YAML_CONVERT(PK::Math::ulong3)
    PK_DECLARE_YAML_CONVERT(PK::Math::ulong4)

    PK_DECLARE_YAML_CONVERT(PK::Math::bool2)
    PK_DECLARE_YAML_CONVERT(PK::Math::bool3)
    PK_DECLARE_YAML_CONVERT(PK::Math::bool4)

    PK_DECLARE_YAML_CONVERT(PK::Math::quaternion)

    #undef PK_DECLARE_YAML_CONVERT
}