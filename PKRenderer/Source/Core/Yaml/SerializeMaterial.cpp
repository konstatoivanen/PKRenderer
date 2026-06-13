#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/Material.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    template<>
    bool Read<Material*>(const ConstNode& node, Material** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Material>(path).get();
        return *rhs != nullptr;
    }

    template<>
    bool Read<MaterialRef>(const ConstNode& node, MaterialRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Material>(path);
        return *rhs != nullptr;
    }

    template<>
    bool Read<MaterialTarget>(const ConstNode& node, MaterialTarget* rhs)
    {
        if (node.is_flow_sl() && node.num_children() == 2u)
        {
            auto pathsubstr = node[0].val();
            FixedString128 path(pathsubstr.len, pathsubstr.data());
            rhs->material = AssetDatabase::Get()->Load<Material>(path);
            node[1] >> rhs->submesh;
        }

        return rhs->material != nullptr;
    }

    template<>
    void Write<Material*>(Node& parent, const char* memberName, Material* const* rhs)
    {
        parent[memberName] << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<MaterialRef>(Node& parent, const char* memberName, const MaterialRef* rhs)
    {
        parent[memberName] << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<MaterialTarget>(Node& parent, const char* memberName, const MaterialTarget* rhs)
    {
        auto node = memberName ? parent[memberName] : parent;
        node |= ryml::SEQ | ryml::FLOW_SL;
        node.append_child() << rhs->material->GetFileName() |= ryml::VAL_DQUO;
        node.append_child() << rhs->submesh;
    }

    PK_YAML_DECLARE_READ_MEMBER(Material*)
    PK_YAML_DECLARE_READ_MEMBER(MaterialRef)
    PK_YAML_DECLARE_READ_MEMBER(MaterialTarget)
}
