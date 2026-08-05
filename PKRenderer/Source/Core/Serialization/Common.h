#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "ISerializer.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/TypeMeta.h"
#include "Core/Base/Reflect.h"
#include "Core/Base/FileIO.h"

namespace PK::Serialize
{
    template<typename T>
    void ReadVal(SerialNodeRead node, T* rhs)
    {
        if (node.readable())
        {
            if constexpr (TSerializeReadableVal<T>)
            {
                ISerializer<T>::ReadVal(node, rhs);
            }
            else
            {
                ReflectFields(*rhs, [node](const char* name, auto& value)
                {
                    ReadVal<PK::TRemoveCVRef_T<decltype(value)>>(node.find_child(name), &value);
                });
            }
        }
    }

    template<typename T>
    void WriteVal(SerialNodeWrite node, const T* rhs)
    {
        if constexpr (TSerializeWritableVal<T>)
        {
            ISerializer<T>::WriteVal(node, rhs);
        }
        else
        {
            ReflectFields(*rhs, [&node](const char* name, auto& value)
            {
                WriteVal<PK::TRemoveCVRef_T<decltype(value)>>(node[name],&value);
            });
        }
    }

    template<typename T>
    void ReadSingle(SerialNodeRead node, T* rhs)
    {
        if (node.readable())
        {
            if constexpr (TSerializeReadableVal<T>)
            {
                ISerializer<T>::ReadVal(node, rhs);
            }
        }
    }

    template<typename T>
    void WriteSingle(SerialNodeWrite node, const T* rhs)
    {
        if constexpr (TSerializeWritableVal<T>)
        {
            ISerializer<T>::WriteVal(node, rhs);
        }
    }

    template<typename T>
    bool Load(const char* filepath, T* rhs)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(filepath, false, &fileData, &fileSize) == 0)
        {
            auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
            auto root = tree.rootref();
            auto base = root[pk_outer_type_name<T>()];
            ReadVal<T>(base, rhs);
            Memory::Free(fileData);
        }

        return false;
    }

    template<typename T> void ReadVal(SerialNodeRead node, const char* name, T* rhs) { ReadVal(node.find_child(name), rhs); }
    template<typename T> T ReadVal(SerialNodeRead node, const char* name) { T v; ReadVal(node, name, &v); return v; }
    template<typename T> T ReadVal(SerialNodeRead node) { T v; ReadVal(node, &v); return v; }
    template<typename T> void ReadKey(SerialNodeRead node, T* rhs) { ISerializer<T>::ReadKey(node, rhs); }
    template<typename T> T ReadKey(SerialNodeRead node) { T v; ReadKey(node, &v); return v; }
    template<typename T> void WriteVal(SerialNodeWrite parent, const char* name, const T* rhs) { WriteVal(parent[name], rhs); }
    template<typename T> T Load(const char* filepath) { T v; Load(filepath, &v); return v; }
}
#endif
