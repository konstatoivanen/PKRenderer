#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Base/Containers/ArrayList.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    template<typename T, size_t capacity> 
    struct ISerializer<FixedArray<T, capacity>>
    {
        using Type = FixedArray<T, capacity>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            const auto count = node.num_children();
            for (auto i = 0u; i < count && i < capacity; ++i)
            {
                ISerializer<T>::ReadVal(node[i], &(*rhs)[i]);
            }
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < capacity; ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };

    template<typename T, size_t inline_capacity> 
    struct ISerializer<InlineArray<T, inline_capacity>>
    {
        using Type = InlineArray<T, inline_capacity>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            const auto count = node.num_children();
            rhs->Reserve(count, false);

            for (auto i = 0u; i < count; ++i)
            {
                ISerializer<T>::ReadVal(node[i], &(*rhs)[i]);
            }
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < rhs->GetSize(); ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };

    template<typename T>
    struct ISerializer<HeapArray<T>>
    {
        using Type = HeapArray<T>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            const auto count = node.num_children();
            rhs->Reserve(count, false);

            for (auto i = 0u; i < count; ++i)
            {
                ISerializer<T>::ReadVal(node[i], &(*rhs)[i]);
            }
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < rhs->GetSize(); ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };


    template<typename T, size_t capacity> 
    struct ISerializer<FixedList<T, capacity>>
    {
        using Type = FixedList<T, capacity>;

        static void ReadVal(SerialNodeRead node, Type* rhs)
        {
            const auto count = node.num_children();
            rhs->Clear();

            for (auto i = 0u; i < count && i < capacity; ++i)
            {
                ISerializer<T>::ReadVal(node[i], rhs->Add());
            }
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < rhs->GetCount(); ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };

    template<typename T, size_t inline_capacity> 
    struct ISerializer<InlineList<T, inline_capacity>>
    {
        using Type = InlineList<T, inline_capacity>;

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

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node.set_seq(ryml::BLOCK);
            node.clear_children();
            for (auto i = 0u; i < rhs->GetCount(); ++i)
            {
                ISerializer<T>::WriteVal(node.append_child(), &(*rhs)[i]);
            }
        }
    };

    template<typename T>
    struct ISerializer<HeapList<T>>
    {
        using Type = HeapList<T>;

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

        static void WriteVal(SerialNodeWrite node, T const* rhs)
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
