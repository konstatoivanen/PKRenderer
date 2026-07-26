#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/Mesh.h"
#include "Core/Yaml/Serialize.h"

namespace PK::YAML
{
    template<>
    void Read<MeshStatic*>(const ConstNode& node, MeshStatic** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<MeshStatic>(path).get();
    }

    template<>
    void Read<MeshStaticRef>(const ConstNode& node, MeshStaticRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<MeshStatic>(path);
    }

    template<>
    void Read<Mesh*>(const ConstNode& node, Mesh** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Mesh>(path).get();
    }

    template<>
    void Read<MeshRef>(const ConstNode& node, MeshRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Mesh>(path);
    }

    template<> void Write<MeshStatic*>(Node& node, MeshStatic* const* rhs) { node << (*rhs)->GetFileName() |= ryml::VAL_DQUO; }
    template<> void Write<MeshStaticRef>(Node& node, const MeshStaticRef* rhs) { node << (*rhs)->GetFileName() |= ryml::VAL_DQUO; }
    template<> void Write<Mesh*>(Node& node, Mesh* const* rhs) { node << (*rhs)->GetFileName() |= ryml::VAL_DQUO; }
    template<> void Write<MeshRef>(Node& node, const MeshRef* rhs) { node << (*rhs)->GetFileName() |= ryml::VAL_DQUO; }
}
