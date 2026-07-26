#include "PrecompiledHeader.h"
#include "Core/Yaml/Serialize.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/Material.h"

namespace PK::Serialize
{
    template<>
    void Read<Material*>(const ConstNode& node, Material** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Material>(path).get();
    }

    template<>
    void Read<MaterialRef>(const ConstNode& node, MaterialRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<Material>(path);
    }

    template<>
    void Read<MaterialTarget>(const ConstNode& node, MaterialTarget* rhs)
    {
        if (node.is_flow_sl() && node.num_children() == 2u)
        {
            auto pathsubstr = node[0].val();
            FixedString128 path(pathsubstr.len, pathsubstr.data());
            rhs->material = AssetDatabase::Get()->Load<Material>(path);
            node[1] >> rhs->submesh;
        }
    }

    template<>
    void Write<Material*>(Node& node, Material* const* rhs)
    {
        node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<MaterialRef>(Node& node, const MaterialRef* rhs)
    {
        node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<MaterialTarget>(Node& node, const MaterialTarget* rhs)
    {
        node |= ryml::SEQ | ryml::FLOW_SL;
        node.append_child() << rhs->material->GetFileName() |= ryml::VAL_DQUO;
        node.append_child() << rhs->submesh;
    }
}
