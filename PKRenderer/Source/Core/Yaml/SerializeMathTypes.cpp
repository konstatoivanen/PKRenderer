#include "PrecompiledHeader.h"
#include <rapidyaml/ryaml.h>
#include "Core/Yaml/RapidyamlFwd.h"
#include "Core/Math/Math.h"

namespace PK::YAML
{
    #define DECLARE_INTEGRAL_READ(type)                 \
    template<>                                          \
    bool Read<type>(const ConstNode& node, type* rhs)   \
    {                                                   \
        node >> *rhs;                                   \
        return true;                                    \
    }                                                   \
    PK_YAML_DECLARE_READ_MEMBER(type)                   \

    #define DECLARE_VECTOR_READ(type, count)                        \
    template<>                                                      \
    bool Read<type##count>(const ConstNode& node, type##count* rhs) \
    {                                                               \
        if (node.is_flow_sl())                                      \
        {                                                           \
            for (auto i = 0u; i < count; ++i)                       \
            {                                                       \
                node[i] >> (*rhs)[i];                               \
            }                                                       \
        }                                                           \
        return true;                                                \
    }                                                               \
    PK_YAML_DECLARE_READ_MEMBER(type##count)                        \

    #define DECLARE_MATRIX_READ(type, countx, county)										\
    template<>                                                                              \
    bool Read<type##countx##x##county>(const ConstNode& node, type##countx##x##county* rhs) \
    {                                                                                       \
        if (node.is_flow_sl())                                                              \
        {                                                                                   \
            for (auto i = 0u; i < countx; ++i)                                              \
            for (auto j = 0u; j < county; ++j)                                              \
            {                                                                               \
                node[i * county + j] >> (*rhs)[i][j];                                       \
            }                                                                               \
        }                                                                                   \
        return true;                                                                        \
    }                                                                                       \
    PK_YAML_DECLARE_READ_MEMBER(type##countx##x##county)                                    \

    DECLARE_INTEGRAL_READ(bool)
    DECLARE_INTEGRAL_READ(uint8_t)
    DECLARE_INTEGRAL_READ(int8_t)
    DECLARE_INTEGRAL_READ(uint16_t)
    DECLARE_INTEGRAL_READ(int16_t)
    DECLARE_INTEGRAL_READ(int32_t)
    DECLARE_INTEGRAL_READ(uint32_t)
    DECLARE_INTEGRAL_READ(int64_t)
    DECLARE_INTEGRAL_READ(uint64_t)
    DECLARE_INTEGRAL_READ(float)
    DECLARE_INTEGRAL_READ(double)

    DECLARE_VECTOR_READ(float,2)
    DECLARE_VECTOR_READ(float,3)
    DECLARE_VECTOR_READ(float,4)

    DECLARE_VECTOR_READ(double,2)
    DECLARE_VECTOR_READ(double,3)
    DECLARE_VECTOR_READ(double,4)

    DECLARE_VECTOR_READ(short,2)
    DECLARE_VECTOR_READ(short,3)
    DECLARE_VECTOR_READ(short,4)

    DECLARE_VECTOR_READ(ushort,2)
    DECLARE_VECTOR_READ(ushort,3)
    DECLARE_VECTOR_READ(ushort,4)

    DECLARE_VECTOR_READ(byte,4)
    DECLARE_VECTOR_READ(sbyte,4)

    DECLARE_VECTOR_READ(int,2)
    DECLARE_VECTOR_READ(int,3)
    DECLARE_VECTOR_READ(int,4)

    DECLARE_VECTOR_READ(uint,2)
    DECLARE_VECTOR_READ(uint,3)
    DECLARE_VECTOR_READ(uint,4)

    DECLARE_VECTOR_READ(long,2)
    DECLARE_VECTOR_READ(long,3)
    DECLARE_VECTOR_READ(long,4)

    DECLARE_VECTOR_READ(ulong,2)
    DECLARE_VECTOR_READ(ulong,3)
    DECLARE_VECTOR_READ(ulong,4)

    DECLARE_VECTOR_READ(bool,2)
    DECLARE_VECTOR_READ(bool,3)
    DECLARE_VECTOR_READ(bool,4)

    DECLARE_MATRIX_READ(float,2,2)
    DECLARE_MATRIX_READ(float,3,3)
    DECLARE_MATRIX_READ(float,3,4)
    DECLARE_MATRIX_READ(float,4,4)

    DECLARE_MATRIX_READ(double,2,2)
    DECLARE_MATRIX_READ(double,3,3)
    DECLARE_MATRIX_READ(double,3,4)
    DECLARE_MATRIX_READ(double,4,4)

    DECLARE_MATRIX_READ(ushort,2,2)
    DECLARE_MATRIX_READ(ushort,3,3)
    DECLARE_MATRIX_READ(ushort,3,4)
    DECLARE_MATRIX_READ(ushort,4,4)

#undef DECLARE_INTEGRAL_READ
#undef DECLARE_VECTOR_READ
#undef DECLARE_MATRIX_READ
}