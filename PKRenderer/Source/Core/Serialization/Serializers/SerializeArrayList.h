#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Base/Containers/ArrayList.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template<typename T, typename TAllocation> 
    struct ISerializer<Array<T, TAllocation>, void>
    {
        using Type = Array<T, TAllocation>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            const auto count = node.num_children();
            rhs->Reserve(count, false);

            for (auto i = 0u; i < count; ++i)
            {
                ISerializer<T>::ReadVal(node[i], &(*rhs)[i]);
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < rhs->GetSize(); ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };

    template<typename T, typename TAllocation>
    struct ISerializer<List<T, TAllocation>, void>
    {
        using Type = List<T, TAllocation>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            const auto count = node.num_children();
            rhs->Reserve(count, false);
            rhs->Clear();

            for (auto i = 0u; i < count; ++i)
            {
                ISerializer<T>::ReadVal(node[i], rhs->Add());
            }
        }

        static void WriteVal(SerialNodeWrite node, Type const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < rhs->GetCount(); ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };
}
#endif
