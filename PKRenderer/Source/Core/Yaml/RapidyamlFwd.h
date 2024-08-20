#pragma once

namespace c4
{
    template<class C> struct basic_substring;
    using csubstr = basic_substring<const char>;
    using substr = basic_substring<char>;

    namespace yml
    {
        class ConstNodeRef;
        class NodeRef;
    }
}

namespace ryml
{
    using namespace c4::yml;
    using namespace c4;
}

namespace PK::YAML
{
    typedef ryml::ConstNodeRef ConstNode;

    template<typename T>
    bool Read(const ConstNode& node, T* rhs);

    template<typename T>
    bool Read(const ConstNode& node, const char* memberName, T* rhs);

    template<typename T>
    bool ReadKey(const ConstNode& node, T* rhs);

    template<typename T>
    T Read(const ConstNode& node)
    {
        T outValue;
        Read(node, &outValue);
        return outValue;
    }

    template<typename T>
    T ReadKey(const ConstNode& node)
    {
        T outValue;
        ReadKey(node, &outValue);
        return outValue;
    }

    // @TODO No write support as of now.
    //template<typename T>
    //void Write(YamlNode* node, const T& rhs);
}

#define PK_YAML_DECLARE_READ_MEMBER(type) \
template<> bool Read<type>(const ConstNode& node, const char* name, type* rhs) { auto member = node.find_child(name); if (member.readable()) return Read(member, rhs); return true; } \
