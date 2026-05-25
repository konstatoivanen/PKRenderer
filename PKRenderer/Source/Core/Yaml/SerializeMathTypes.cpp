#include "PrecompiledHeader.h"
#include "Core/Math/Math.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    #define DECLARE_SCALAR_SERIALIZE(type)                                                                                                  \
    template<> bool Read<type>(const ConstNode& node, type* rhs) { node >> *rhs; return true; }                                             \
    template<> void Write<type>(Node& parent, const char* memberName, const type* rhs) { parent[memberName] << *rhs |= ryml::VAL_PLAIN; }   \
    PK_YAML_DECLARE_READ_MEMBER(type)                                                                                                       \


    #define DECLARE_VECTOR_SERIALIZE(type, count)                                           \
    template<>                                                                              \
    bool Read<type##count>(const ConstNode& node, type##count* rhs)                         \
    {                                                                                       \
        if (node.is_flow_sl() && node.num_children() > 0u)                                  \
        {                                                                                   \
            auto maxidx = (uint32_t)node.num_children() - 1u;                               \
            for (auto i = 0u; i < count; ++i)                                               \
            {                                                                               \
                node[math::min(maxidx, i)] >> (*rhs)[i];                                    \
            }                                                                               \
        }                                                                                   \
        return true;                                                                        \
    }                                                                                       \
    template<>                                                                              \
    void Write<type##count>(Node& parent, const char* memberName, const type##count* rhs)   \
    {                                                                                       \
        auto node = parent[memberName];                                                     \
        node |= ryml::SEQ | ryml::FLOW_SL;                                                  \
        for (auto i = 0u; i < count; ++i)                                                   \
        {                                                                                   \
            node.append_child() << (*rhs)[i];                                               \
        }                                                                                   \
    }                                                                                       \
    PK_YAML_DECLARE_READ_MEMBER(type##count)                                                \


    #define DECLARE_MATRIX_SERIALIZE(type, countx, county)										                    \
    template<>                                                                                                      \
    bool Read<type##countx##x##county>(const ConstNode& node, type##countx##x##county* rhs)                         \
    {                                                                                                               \
        if (node.is_flow_sl() && node.num_children() == (countx * county))                                          \
        {                                                                                                           \
            for (auto y = 0u; y < county; ++y)                                                                      \
            for (auto x = 0u; x < countx; ++x)                                                                      \
            {                                                                                                       \
                node[x + y * countx] >> (*rhs)[y][x];                                                               \
            }                                                                                                       \
        }                                                                                                           \
        return true;                                                                                                \
    }                                                                                                               \
    template<>                                                                                                      \
    void Write<type##countx##x##county>(Node& parent, const char* memberName, const type##countx##x##county* rhs)   \
    {                                                                                                               \
        auto node = parent[memberName];                                                                             \
        node |= ryml::SEQ | ryml::FLOW_SL;                                                                          \
        for (auto y = 0u; y < county; ++y)                                                                          \
        for (auto x = 0u; x < countx; ++x)                                                                          \
        {                                                                                                           \
            node.append_child() << (*rhs)[y][x];                                                                    \
        }                                                                                                           \
    }                                                                                                               \
    PK_YAML_DECLARE_READ_MEMBER(type##countx##x##county)                                                            \

    DECLARE_SCALAR_SERIALIZE(bool)
    DECLARE_SCALAR_SERIALIZE(uint8_t)
    DECLARE_SCALAR_SERIALIZE(int8_t)
    DECLARE_SCALAR_SERIALIZE(uint16_t)
    DECLARE_SCALAR_SERIALIZE(int16_t)
    DECLARE_SCALAR_SERIALIZE(int32_t)
    DECLARE_SCALAR_SERIALIZE(uint32_t)
    DECLARE_SCALAR_SERIALIZE(int64_t)
    DECLARE_SCALAR_SERIALIZE(uint64_t)
    DECLARE_SCALAR_SERIALIZE(float)
    DECLARE_SCALAR_SERIALIZE(double)

    DECLARE_VECTOR_SERIALIZE(float,2)
    DECLARE_VECTOR_SERIALIZE(float,3)
    DECLARE_VECTOR_SERIALIZE(float,4)

    DECLARE_VECTOR_SERIALIZE(double,2)
    DECLARE_VECTOR_SERIALIZE(double,3)
    DECLARE_VECTOR_SERIALIZE(double,4)

    DECLARE_VECTOR_SERIALIZE(short,2)
    DECLARE_VECTOR_SERIALIZE(short,3)
    DECLARE_VECTOR_SERIALIZE(short,4)

    DECLARE_VECTOR_SERIALIZE(ushort,2)
    DECLARE_VECTOR_SERIALIZE(ushort,3)
    DECLARE_VECTOR_SERIALIZE(ushort,4)

    DECLARE_VECTOR_SERIALIZE(byte,4)
    DECLARE_VECTOR_SERIALIZE(sbyte,4)

    DECLARE_VECTOR_SERIALIZE(int,2)
    DECLARE_VECTOR_SERIALIZE(int,3)
    DECLARE_VECTOR_SERIALIZE(int,4)

    DECLARE_VECTOR_SERIALIZE(uint,2)
    DECLARE_VECTOR_SERIALIZE(uint,3)
    DECLARE_VECTOR_SERIALIZE(uint,4)

    DECLARE_VECTOR_SERIALIZE(long,2)
    DECLARE_VECTOR_SERIALIZE(long,3)
    DECLARE_VECTOR_SERIALIZE(long,4)

    DECLARE_VECTOR_SERIALIZE(ulong,2)
    DECLARE_VECTOR_SERIALIZE(ulong,3)
    DECLARE_VECTOR_SERIALIZE(ulong,4)

    DECLARE_VECTOR_SERIALIZE(bool,2)
    DECLARE_VECTOR_SERIALIZE(bool,3)
    DECLARE_VECTOR_SERIALIZE(bool,4)

    DECLARE_MATRIX_SERIALIZE(float,2,2)
    DECLARE_MATRIX_SERIALIZE(float,3,3)
    DECLARE_MATRIX_SERIALIZE(float,3,4)
    DECLARE_MATRIX_SERIALIZE(float,4,4)

    DECLARE_MATRIX_SERIALIZE(double,2,2)
    DECLARE_MATRIX_SERIALIZE(double,3,3)
    DECLARE_MATRIX_SERIALIZE(double,3,4)
    DECLARE_MATRIX_SERIALIZE(double,4,4)

    DECLARE_MATRIX_SERIALIZE(ushort,2,2)
    DECLARE_MATRIX_SERIALIZE(ushort,3,3)
    DECLARE_MATRIX_SERIALIZE(ushort,3,4)
    DECLARE_MATRIX_SERIALIZE(ushort,4,4)

#undef DECLARE_SCALAR_SERIALIZE
#undef DECLARE_VECTOR_SERIALIZE
#undef DECLARE_MATRIX_SERIALIZE
}