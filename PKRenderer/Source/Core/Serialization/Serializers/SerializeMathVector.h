#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"
#include "Core/Math/Math.h"

namespace PK
{
    template<typename T, int N>
    struct ISerializer<math::vector<T, N>>
    {
        using Type = math::vector<T, N>;

        static void ReadVal(const SerialNodeConst& node, Type* rhs)
        {
            if (node.is_flow_sl() && node.num_children() > 0u)
            {
                auto maxidx = (uint32_t)node.num_children() - 1u;
                for (auto i = 0u; i < N; ++i)
                {
                    node[math::min(maxidx, i)] >> (*rhs)[i];
                }
            }
        }

        static void WriteVal(SerialNode& node, const Type* rhs)
        {
            node |= ryml::SEQ | ryml::FLOW_SL;
            for (auto i = 0u; i < N; ++i)
            {
                node.append_child() << (*rhs)[i];
            }
        }
    };

    template<typename T, int C, int R>
    struct ISerializer<math::matrix<T, C, R>>
    {
        using Type = math::matrix<T, C, R>;

        static void ReadVal(const SerialNodeConst& node, Type* rhs)
        {
            if (node.is_flow_sl() && node.num_children() == (C * R))
            {
                for (auto y = 0u; y < R; ++y)
                for (auto x = 0u; x < C; ++x)
                {
                    node[x + y * C] >> (*rhs)[y][x];
                }
            }
        }

        static void WriteVal(SerialNode& node, Type const* rhs)
        {
            node |= ryml::SEQ | ryml::FLOW_SL;
            for (auto y = 0u; y < R; ++y)
            for (auto x = 0u; x < C; ++x)
            {
                node.append_child() << (*rhs)[y][x];
            }
        }
    };
}
#endif
