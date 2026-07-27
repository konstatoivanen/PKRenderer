#pragma once
#if !PK_DEBUG
#define C4_USE_ASSERT 0
#endif
#include <rapidyaml/ryaml.h>
#include "Core/Utilities/TypeIndex.h"
#include "Core/Utilities/Reflect.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/FileIO.h"

namespace PK::Serialize
{
    typedef ryml::ConstNodeRef ConstNode;
    typedef ryml::NodeRef Node;

    template<typename T> void Read(const ConstNode& node, T* rhs) = delete;
    template<typename T> void Write(Node& node, const T* rhs) = delete;
    // This is a bit bad as it creates an assumption that keys can be parsed to anything other than strings.
    template<typename T> void ReadKey(const ConstNode& node, T* rhs) = delete;

    template<typename T> concept TReadable = requires(const ConstNode & node, T * rhs) { Read(node, rhs); };
    template<typename T> concept TTKeyReadable = requires(const ConstNode & node, T * rhs) { ReadKey(node, rhs); };
    template<typename T> concept TWritable = requires(Node & node, const T * rhs) { Write(node, rhs); };
}

#define PK_CUSTOM_SERIALIZABLES
#include "Serializables.h"
#undef PK_CUSTOM_SERIALIZERS

namespace PK::Serialize
{
    template<typename T>
    void Read(const ConstNode& node, const char* memberName, T* rhs)
    {
        const auto member = node.find_child(memberName);
        
        if (member.readable())
        {
            Read(member, rhs);
        }
    }

    template<typename T> 
    T Read(const ConstNode& node) 
    { 
        T value; 
        Read(node, &value); 
        return value; 
    }

    template<typename T>
    T Read(const ConstNode& node, const char* memberName)
    {
        T value;
        Read(node, memberName, &value);
        return value;
    }
    
    template<typename T> 
    void Write(Node& parent, const char* memberName, const T* rhs)
    {
        Node node = parent[memberName];
        Write(node, rhs);
    }

    template<typename T>
    T ReadKey(const ConstNode& node)
    {
        T outValue;
        ReadKey(node, &outValue);
        return outValue;
    }

    template<typename T>
    void ReadStruct(const ConstNode& node, T* rhs)
    {
        ReflectFields(*rhs, [node](const char* name, auto& value)
        {
            using TField = PK::TRemoveCVRef_T<decltype(value)>;

            auto member = node.find_child(name);

            // Allow declaring members with absolute path in relation to type name so that configs can be more readable.
            if (!member.readable())
            {
                FixedString64 absoluteName("%s.%s", pk_outer_type_name<T>(), name);
                member = node.find_child(absoluteName.c_str());
            }

            if (member.readable())
            {
                if constexpr (TReadable<TField>)
                {
                    Read<TField>(member, &value);
                }
                else
                {
                    ReadStruct<TField>(member, &value);
                }
            }
        });
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
        Serialize::ConstNode root = tree.rootref();

        ReadStruct(root, rhs);

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
