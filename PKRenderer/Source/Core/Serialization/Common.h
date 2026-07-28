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
    void ReadVal(const SerialNodeConst& node, T* rhs)
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
    void ReadVal(const SerialNodeConst& node, const char* memberName, T* rhs)
    {
        ReadVal(node.find_child(memberName), rhs);
    }

    template<typename T> 
    T ReadVal(const SerialNodeConst& node)
    { 
        T value; 
        ReadVal(node, &value); 
        return value; 
    }

    template<typename T>
    T ReadVal(const SerialNodeConst& node, const char* memberName)
    {
        T value;
        ReadVal(node, memberName, &value);
        return value;
    }

    template<typename T>
    void ReadKey(const SerialNodeConst& node, T* rhs)
    {
        if constexpr (TSerializeReadableKey<T>)
        {
            ISerializer<T>::ReadKey(node, rhs);
        }
    }

    template<typename T>
    T ReadKey(const SerialNodeConst& node)
    {
        T outValue;
        ReadKey(node, &outValue);
        return outValue;
    }

    template<typename T>
    void WriteVal(SerialNode& node, const T* rhs)
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
                SerialNode child = node[FixedString64("%s.%s", pk_outer_type_name<T>(), name).c_str()];
                WriteVal<PK::TRemoveCVRef_T<decltype(value)>>(child, &value);
            });
        }
    }

    template<typename T> 
    void WriteVal(SerialNode& parent, const char* memberName, const T* rhs)
    {
        SerialNode child = parent[memberName];
        WriteVal(child, rhs);
    }

    template<typename T>
    bool LoadStruct(const char* filepath, T* rhs)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(filepath, false, &fileData, &fileSize) != 0)
        {
            return false;
        }

        auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
        SerialNodeConst root = tree.rootref();

        ReadVal<T>(root, rhs);

        Memory::Free(fileData);
        return true;
    }

    template<typename T>
    T LoadStruct(const char* filepath)
    {
        T value;
        LoadStruct(filepath, &value);
        return value;
    }
}
#endif
