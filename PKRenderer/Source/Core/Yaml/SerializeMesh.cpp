#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/Mesh.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    template<>
    bool Read<MeshStatic*>(const ConstNode& node, MeshStatic** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<MeshStatic>(path).get();
        return *rhs != nullptr;
    }

    template<>
    bool Read<MeshStaticRef>(const ConstNode& node, MeshStaticRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<MeshStatic>(path);
        return *rhs != nullptr;
    }

    template<>
    bool Read<Mesh*>(const ConstNode& node, Mesh** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Mesh>(path).get();
        return *rhs != nullptr;
    }

    template<>
    bool Read<MeshRef>(const ConstNode& node, MeshRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Mesh>(path);
        return *rhs != nullptr;
    }

    template<>
    void Write<MeshStatic*>(Node& parent, const char* memberName, MeshStatic* const* rhs)
    {
        parent[memberName] << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<MeshStaticRef>(Node& parent, const char* memberName, const MeshStaticRef* rhs)
    {
        parent[memberName] << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<Mesh*>(Node& parent, const char* memberName, Mesh* const* rhs)
    {
        parent[memberName] << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<MeshRef>(Node& parent, const char* memberName, const MeshRef* rhs)
    {
        parent[memberName] << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    PK_YAML_DECLARE_READ_MEMBER(MeshStatic*)
    PK_YAML_DECLARE_READ_MEMBER(MeshStaticRef)
    PK_YAML_DECLARE_READ_MEMBER(Mesh*)
    PK_YAML_DECLARE_READ_MEMBER(MeshRef)
}
