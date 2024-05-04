#include "PrecompiledHeader.h"
#include "ConvertMathTypes.h"

namespace YAML
{
    using namespace PK::Math;

#define DECLARE_VECTOR_CONVERT(type, count)								\
    Node convert<type##count>::encode(const type##count & rhs)				\
    {																		\
        Node node;															\
        for (auto i = 0; i < count; ++i)									\
        {																	\
            node.push_back(rhs[i]);											\
        }																	\
        node.SetStyle(EmitterStyle::Flow);									\
        return node;														\
    }																		\
                                                                            \
    bool convert<type##count>::decode(const Node& node, type##count & rhs)	\
    {																		\
        if (!node.IsSequence() || node.size() != count)						\
        {																	\
            return false;													\
        }																	\
                                                                            \
        for (auto i = 0; i < count; ++i)									\
        {																	\
            rhs[i] = node[i].as<type>();									\
        }																	\
                                                                            \
        return true;														\
    }																		\

    DECLARE_VECTOR_CONVERT(float, 2)
    DECLARE_VECTOR_CONVERT(float, 3)
    DECLARE_VECTOR_CONVERT(float, 4)

    DECLARE_VECTOR_CONVERT(double, 2)
    DECLARE_VECTOR_CONVERT(double, 3)
    DECLARE_VECTOR_CONVERT(double, 4)

    DECLARE_VECTOR_CONVERT(short, 2)
    DECLARE_VECTOR_CONVERT(short, 3)
    DECLARE_VECTOR_CONVERT(short, 4)

    DECLARE_VECTOR_CONVERT(ushort, 2)
    DECLARE_VECTOR_CONVERT(ushort, 3)
    DECLARE_VECTOR_CONVERT(ushort, 4)

    DECLARE_VECTOR_CONVERT(byte, 4)
    DECLARE_VECTOR_CONVERT(sbyte, 4)

    DECLARE_VECTOR_CONVERT(int, 2)
    DECLARE_VECTOR_CONVERT(int, 3)
    DECLARE_VECTOR_CONVERT(int, 4)

    DECLARE_VECTOR_CONVERT(uint, 2)
    DECLARE_VECTOR_CONVERT(uint, 3)
    DECLARE_VECTOR_CONVERT(uint, 4)

    DECLARE_VECTOR_CONVERT(long, 2)
    DECLARE_VECTOR_CONVERT(long, 3)
    DECLARE_VECTOR_CONVERT(long, 4)

    DECLARE_VECTOR_CONVERT(ulong, 2)
    DECLARE_VECTOR_CONVERT(ulong, 3)
    DECLARE_VECTOR_CONVERT(ulong, 4)

    DECLARE_VECTOR_CONVERT(bool, 2)
    DECLARE_VECTOR_CONVERT(bool, 3)
    DECLARE_VECTOR_CONVERT(bool, 4)

#undef DECLARE_VECTOR_CONVERT

#define DECLARE_MATRIX_CONVERTER(type, countx, county)											\
    Node convert<type##countx##x##county>::encode(const type##countx##x##county & rhs)				\
    {																								\
        Node node;																					\
        for (auto i = 0; i < countx; ++i)															\
        for (auto j = 0; j < county; ++j)															\
        {																							\
            node.push_back(rhs[i][j]);																\
        }																							\
        node.SetStyle(EmitterStyle::Flow);															\
        return node;																				\
    }																								\
                                                                                                    \
    bool convert<type##countx##x##county>::decode(const Node& node, type##countx##x##county & rhs)	\
    {																								\
        if (!node.IsSequence() || node.size() != countx * county)									\
        {																							\
            return false;																			\
        }																							\
                                                                                                    \
        for (auto i = 0; i < countx; ++i)															\
        for (auto j = 0; j < county; ++j)															\
        {																							\
            rhs[i][j] = node[i * county + j].as<type>();											\
        }																							\
                                                                                                    \
        return true;																				\
    }																								\

    DECLARE_MATRIX_CONVERTER(float, 2, 2)
    DECLARE_MATRIX_CONVERTER(float, 3, 3)
    DECLARE_MATRIX_CONVERTER(float, 4, 4)
    DECLARE_MATRIX_CONVERTER(float, 3, 4)

    DECLARE_MATRIX_CONVERTER(double, 2, 2)
    DECLARE_MATRIX_CONVERTER(double, 3, 3)
    DECLARE_MATRIX_CONVERTER(double, 4, 4)

    DECLARE_MATRIX_CONVERTER(ushort, 2, 2)
    DECLARE_MATRIX_CONVERTER(ushort, 3, 3)
    DECLARE_MATRIX_CONVERTER(ushort, 4, 4)

    Node convert<quaternion>::encode(const quaternion& rhs)
    {
        Node node;
        for (auto i = 0; i < 4; ++i)
        {
            node.push_back(rhs[i]);
        }

        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    bool convert<quaternion>::decode(const Node& node, quaternion& rhs)
    {
        if (!node.IsSequence() || node.size() != 4)
        {
            return false;
        }

        for (auto i = 0; i < 4; ++i)
        {
            rhs[i] = node[i].as<float>();
        }

        return true;
    }
}