#include "PrecompiledHeader.h"
#include "Core/Yaml/Serialize.h"
#include "Core/Math/Math.h"

namespace PK::Serialize
{
    #define DECLARE_SCALAR_SERIALIZE(type)                                                          \
    template<> void Read<type>(const ConstNode& node, type* rhs) { node >> *rhs; }                  \
    template<> void Write<type>(Node& node, const type* rhs) { node<< *rhs |= ryml::VAL_PLAIN; }    \


    #define DECLARE_VECTOR_SERIALIZE(type, count)                   \
    template<>                                                      \
    void Read<type##count>(const ConstNode& node, type##count* rhs) \
    {                                                               \
        if (node.is_flow_sl() && node.num_children() > 0u)          \
        {                                                           \
            auto maxidx = (uint32_t)node.num_children() - 1u;       \
            for (auto i = 0u; i < count; ++i)                       \
            {                                                       \
                node[math::min(maxidx, i)] >> (*rhs)[i];            \
            }                                                       \
        }                                                           \
    }                                                               \
    template<>                                                      \
    void Write<type##count>(Node& node, const type##count* rhs)     \
    {                                                               \
        node |= ryml::SEQ | ryml::FLOW_SL;                          \
        for (auto i = 0u; i < count; ++i)                           \
        {                                                           \
            node.append_child() << (*rhs)[i];                       \
        }                                                           \
    }                                                               \


    #define DECLARE_MATRIX_SERIALIZE(type, countx, county)									\
    template<>                                                                              \
    void Read<type##countx##x##county>(const ConstNode& node, type##countx##x##county* rhs) \
    {                                                                                       \
        if (node.is_flow_sl() && node.num_children() == (countx * county))                  \
        {                                                                                   \
            for (auto y = 0u; y < county; ++y)                                              \
            for (auto x = 0u; x < countx; ++x)                                              \
            {                                                                               \
                node[x + y * countx] >> (*rhs)[y][x];                                       \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
    template<>                                                                              \
    void Write<type##countx##x##county>(Node& node, const type##countx##x##county* rhs)     \
    {                                                                                       \
        node |= ryml::SEQ | ryml::FLOW_SL;                                                  \
        for (auto y = 0u; y < county; ++y)                                                  \
        for (auto x = 0u; x < countx; ++x)                                                  \
        {                                                                                   \
            node.append_child() << (*rhs)[y][x];                                            \
        }                                                                                   \
    }                                                                                       \

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
