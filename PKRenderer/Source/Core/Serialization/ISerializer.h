#pragma once
#ifdef PK_SERIALIZE_HEADER
#if !PK_DEBUG
#define C4_USE_ASSERT 0
#endif
#include <rapidyaml/ryaml.h>

namespace PK
{
    typedef ryml::ConstNodeRef SerialNodeRead;
    typedef ryml::NodeRef SerialNodeWrite;

    template<typename T, typename = void>
    struct ISerializer
    {
        // Function interface example.
        static void ReadVal(SerialNodeRead node, T* rhs) = delete;
        static void ReadKey(SerialNodeRead node, T* rhs) = delete;
        static void WriteVal(SerialNodeWrite node, T const* rhs) = delete;
        static void WriteKey(SerialNodeWrite node, T const* rhs) = delete;
    };

    template<typename T> concept TSerializeReadableVal = requires(SerialNodeRead node, T * rhs) { ISerializer<T>::ReadVal(node, rhs); };
    template<typename T> concept TSerializeReadableKey = requires(SerialNodeRead node, T * rhs) { ISerializer<T>::ReadKey(node, rhs); };
    template<typename T> concept TSerializeWritableVal = requires(SerialNodeWrite node, T const* rhs) { ISerializer<T>::WriteVal(node, rhs); };
    template<typename T> concept TSerializeWritableKey = requires(SerialNodeWrite node, T const* rhs) { ISerializer<T>::WriteKey(node, rhs); };
}
#endif
