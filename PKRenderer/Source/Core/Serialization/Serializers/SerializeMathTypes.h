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
            node.clear_children();
            for (auto i = 0u; i < N; ++i)
            {
                node.append_child().save((*rhs)[i], ryml::VAL_PLAIN);
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
            node.clear_children();
            for (auto i = 0u; i < 4u; ++i)
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
            for (auto c = 0u; c < C && node.num_children() == C * R; ++c)
            for (auto r = 0u; r < R; ++r)
            {
                node[c * R + r].load(&((*rhs)[c][r]), false);
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::FLOW_SL);
            node.clear_children();
            for (auto c = 0u; c < C; ++c)
            for (auto r = 0u; r < R; ++r)
            {
                node.append_child().save((*rhs)[c][r], ryml::VAL_PLAIN);
            }
        }
    };

    template<typename T, int N>
    struct ISerializer<math::AABB<T,N>>
    {
        using Type = math::AABB<T,N>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            if (node.num_children() == 2 * N)
            {
                for (auto i = 0u; i < N; ++i) node[0u * N + i].load(&(rhs->min[i]), false);
                for (auto i = 0u; i < N; ++i) node[1u * N + i].load(&(rhs->max[i]), false);
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::FLOW_SL);
            node.clear_children();
            for (auto i = 0u; i < N; ++i) node.append_child().save(rhs->min[i], ryml::VAL_PLAIN);
            for (auto i = 0u; i < N; ++i) node.append_child().save(rhs->max[i], ryml::VAL_PLAIN);
        }
    };
}
#endif
