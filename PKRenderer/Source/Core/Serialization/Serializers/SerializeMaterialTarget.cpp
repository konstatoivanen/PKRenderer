#include "PrecompiledHeader.h"
#include "Core/Serialization/Serialize.h"
#include "Core/Rendering/Material.h"

namespace PK
{
    void ISerializer<MaterialTarget>::ReadVal(const SerialNodeConst& node, MaterialTarget* rhs)
    {
        if (node.is_flow_sl() && node.num_children() == 2u)
        {
            auto pathsubstr = node[0].val();
            FixedString128 path(pathsubstr.len, pathsubstr.data());
            rhs->material = AssetDatabase::Get()->Load<Material>(path);
            node[1] >> rhs->submesh;
        }
    }

    void ISerializer<MaterialTarget>::WriteVal(SerialNode& node, const MaterialTarget* rhs)
    {
        node |= ryml::SEQ | ryml::FLOW_SL;
        node.append_child() << rhs->material->GetFileName() |= ryml::VAL_DQUO;
        node.append_child() << rhs->submesh;
    }
}
