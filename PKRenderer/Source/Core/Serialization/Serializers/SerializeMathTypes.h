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

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            if (node.is_flow_sl() && node.num_children() > 0u)
            {
                auto maxidx = (uint32_t)node.num_children() - 1u;
                for (auto i = 0u; i < N; ++i)
                {
                    node[math::min(maxidx, i)].load(&((*rhs)[i]), false);
                }
            }
        }

        static void WriteVal(SerialNodeWrite node, const Type* rhs)
        {
            node.set_seq(ryml::FLOW_SL);
            for (auto i = 0u; i < N; ++i)
            {
                node.append_child().save((*rhs)[i], ryml::VAL_PLAIN);
            }
        }
    };

    template<typename T, int C, int R>
    struct ISerializer<math::matrix<T, C, R>>
    {
        using Type = math::matrix<T, C, R>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            if (node.is_flow_sl() && node.num_children() == (C * R))
            {
                for (auto y = 0u; y < R; ++y)
                for (auto x = 0u; x < C; ++x)
                {
                    node[x + y * C].load(&((*rhs)[y][x]), false);
                }
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::FLOW_SL);
            for (auto y = 0u; y < R; ++y)
            for (auto x = 0u; x < C; ++x)
            {
                node.append_child().save((*rhs)[y][x], ryml::VAL_PLAIN);
            }
        }
    };

    template<typename T>
    struct ISerializer<math::quaternion<T>>
    {
        using Type = math::quaternion<T>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            if (node.is_flow_sl() && node.num_children() == 4u)
            {
                for (auto i = 0u; i < 4u; ++i)
                {
                    node[i].load(&((*rhs)[i]), false);
                }
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::FLOW_SL);
            for (auto i = 0u; i < 4u; ++i)
            {
                node.append_child().save((*rhs)[i], ryml::VAL_PLAIN);
            }
        }
    };

    template<typename T, int N>
    struct ISerializer<math::AABB<T,N>>
    {
        using Type = math::AABB<T,N>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            if (node.is_flow_sl() && node.num_children() == (N * 2))
            {
                for (auto i = 0u; i < N; ++i) node[i + N * 0u].load(&(rhs->min[i]), false);
                for (auto i = 0u; i < N; ++i) node[i + N * 1u].load(&(rhs->max[i]), false);
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::FLOW_SL);
            for (auto i = 0u; i < N; ++i) node.append_child().save(rhs->min[i], ryml::VAL_PLAIN);
            for (auto i = 0u; i < N; ++i) node.append_child().save(rhs->max[i], ryml::VAL_PLAIN);
        }
    };
}
#endif
