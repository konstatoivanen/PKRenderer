#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "ISerializer.h"
#include "Core/Utilities/TypeIndex.h"
#include "Core/Utilities/Reflect.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/FileIO.h"

namespace PK::Serialize
{
    template<typename T>
    void ReadVal(SerialNodeRead node, T* rhs)
    {
        if (node.readable())
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                node >> *rhs;
            }
            else if constexpr (TSerializeReadableVal<T>)
            {
                ISerializer<T>::ReadVal(node, rhs);
            }
            else
            {
                ReflectFields(*rhs, [node](const char* name, auto& value)
                {
                    auto member = node.find_child(name);

                    // Allow declaring members with absolute path in relation to type name so that configs can be more readable.
                    if (!member.readable())
                    {
                        member = node.find_child(FixedString64("%s.%s", pk_outer_type_name<T>(), name).c_str());
                    }

                    ReadVal<PK::TRemoveCVRef_T<decltype(value)>>(member, &value);
                });
            }
        }
    }

    template<typename T>
    void ReadVal(SerialNodeRead node, const char* memberName, T* rhs)
    {
        ReadVal(node.find_child(memberName), rhs);
    }

    template<typename T> 
    T ReadVal(SerialNodeRead node)
    { 
        T value; 
        ReadVal(node, &value); 
        return value; 
    }

    template<typename T>
    T ReadVal(SerialNodeRead node, const char* memberName)
    {
        T value;
        ReadVal(node, memberName, &value);
        return value;
    }

    template<typename T>
    void ReadKey(SerialNodeRead node, T* rhs)
    {
        if constexpr (TSerializeReadableKey<T>)
        {
            ISerializer<T>::ReadKey(node, rhs);
        }
    }

    template<typename T>
    T ReadKey(SerialNodeRead node)
    {
        T outValue;
        ReadKey(node, &outValue);
        return outValue;
    }

    template<typename T>
    void WriteVal(SerialNodeWrite node, const T* rhs)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            node << *rhs |= ryml::VAL_PLAIN;
        }
        else if constexpr (TSerializeWritableVal<T>)
        {
            ISerializer<T>::WriteVal(node, rhs);
        }
        else
        {
            ReflectFields(*rhs, [&node](const char* name, auto& value)
            {
                WriteVal<PK::TRemoveCVRef_T<decltype(value)>>(
                    node[FixedString64("%s.%s", pk_outer_type_name<T>(), name).c_str()], 
                    &value);
            });
        }
    }

    template<typename T> 
    void WriteVal(SerialNodeWrite parent, const char* memberName, const T* rhs)
    {
        WriteVal(parent[memberName], rhs);
    }

    template<typename T>
    bool Load(const char* filepath, T* rhs)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(filepath, false, &fileData, &fileSize) == 0)
        {
            auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
            ReadVal<T>(tree.rootref(), rhs);
            Memory::Free(fileData);
        }

        return false;
    }

    template<typename T>
    T Load(const char* filepath)
    {
        T value;
        Load(filepath, &value);
        return value;
    }
}
#endif
