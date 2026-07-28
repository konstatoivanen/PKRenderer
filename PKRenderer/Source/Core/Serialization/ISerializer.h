#pragma once
#ifdef PK_SERIALIZE_HEADER
#if !PK_DEBUG
#define C4_USE_ASSERT 0
#endif
#include <rapidyaml/ryaml.h>

namespace PK
{
    typedef ryml::ConstNodeRef SerialNodeConst;
    typedef ryml::NodeRef SerialNode;

    template<typename T, bool TRequirement = true>
    struct ISerializer
    {
        // Function interface example.
        static void ReadVal(const SerialNodeConst& node, T* rhs) = delete;
        static void ReadKey(const SerialNodeConst& node, T* rhs) = delete;
        static void WriteVal(SerialNode& node, T const* rhs) = delete;
        static void WriteKey(SerialNode& node, T const* rhs) = delete;
    };

    template<typename T> concept TSerializeReadableVal = requires(const SerialNodeConst& node, T * rhs) { ISerializer<T>::ReadVal(node, rhs); };
    template<typename T> concept TSerializeReadableKey = requires(const SerialNodeConst& node, T * rhs) { ISerializer<T>::ReadKey(node, rhs); };
    template<typename T> concept TSerializeWritableVal = requires(SerialNode& node, T const* rhs) { ISerializer<T>::WriteVal(node, rhs); };
    template<typename T> concept TSerializeWritableKey = requires(SerialNode& node, T const* rhs) { ISerializer<T>::WriteKey(node, rhs); };
}
#endif
