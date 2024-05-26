#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Math/MathFwd.h"

namespace YAML
{
    #define PK_DECLARE_YAML_CONVERT(type)               \
    template<>                                          \
    struct convert<type>                                \
    {                                                   \
        static Node encode(const type& rhs);            \
        static bool decode(const Node& node, type& rhs);\
    };                                                  \

    PK_DECLARE_YAML_CONVERT(PK::float2)
    PK_DECLARE_YAML_CONVERT(PK::float3)
    PK_DECLARE_YAML_CONVERT(PK::float4)

    PK_DECLARE_YAML_CONVERT(PK::double2)
    PK_DECLARE_YAML_CONVERT(PK::double3)
    PK_DECLARE_YAML_CONVERT(PK::double4)

    PK_DECLARE_YAML_CONVERT(PK::float2x2)
    PK_DECLARE_YAML_CONVERT(PK::float3x3)
    PK_DECLARE_YAML_CONVERT(PK::float4x4)
    PK_DECLARE_YAML_CONVERT(PK::float3x4)

    PK_DECLARE_YAML_CONVERT(PK::double2x2)
    PK_DECLARE_YAML_CONVERT(PK::double3x3)
    PK_DECLARE_YAML_CONVERT(PK::double4x4)

    PK_DECLARE_YAML_CONVERT(PK::short2)
    PK_DECLARE_YAML_CONVERT(PK::short3)
    PK_DECLARE_YAML_CONVERT(PK::short4)

    PK_DECLARE_YAML_CONVERT(PK::ushort2)
    PK_DECLARE_YAML_CONVERT(PK::ushort3)
    PK_DECLARE_YAML_CONVERT(PK::ushort4)

    PK_DECLARE_YAML_CONVERT(PK::ushort2x2)
    PK_DECLARE_YAML_CONVERT(PK::ushort3x3)
    PK_DECLARE_YAML_CONVERT(PK::ushort4x4)

    PK_DECLARE_YAML_CONVERT(PK::sbyte4)
    PK_DECLARE_YAML_CONVERT(PK::byte4)

    PK_DECLARE_YAML_CONVERT(PK::int2)
    PK_DECLARE_YAML_CONVERT(PK::int3)
    PK_DECLARE_YAML_CONVERT(PK::int4)

    PK_DECLARE_YAML_CONVERT(PK::uint2)
    PK_DECLARE_YAML_CONVERT(PK::uint3)
    PK_DECLARE_YAML_CONVERT(PK::uint4)

    PK_DECLARE_YAML_CONVERT(PK::long2)
    PK_DECLARE_YAML_CONVERT(PK::long3)
    PK_DECLARE_YAML_CONVERT(PK::long4)

    PK_DECLARE_YAML_CONVERT(PK::ulong2)
    PK_DECLARE_YAML_CONVERT(PK::ulong3)
    PK_DECLARE_YAML_CONVERT(PK::ulong4)

    PK_DECLARE_YAML_CONVERT(PK::bool2)
    PK_DECLARE_YAML_CONVERT(PK::bool3)
    PK_DECLARE_YAML_CONVERT(PK::bool4)

    PK_DECLARE_YAML_CONVERT(PK::quaternion)

#undef PK_DECLARE_YAML_CONVERT
}