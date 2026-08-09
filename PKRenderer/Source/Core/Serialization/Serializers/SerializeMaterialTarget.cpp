#include "PrecompiledHeader.h"
#include "Core/Serialization/Serialize.h"
#include "Core/Rendering/Material.h"

namespace PK
{
    void ISerializer<MaterialTarget>::ReadVal(SerialNodeRead node, MaterialTarget* rhs)
    {
        if (node.is_flow_sl() && node.num_children() == 2u)
        {
            auto pathsubstr = node[0].val();
            FixedString128 path(pathsubstr.len, pathsubstr.data());
            rhs->material = AssetDatabase::Get()->Load<Material>(path);
            node[1].load(&rhs->submesh, false);
        }
    }

    void ISerializer<MaterialTarget>::WriteVal(SerialNodeWrite node, const MaterialTarget* rhs)
    {
        node.set_seq(ryml::FLOW_SL);
        node.clear_children();
        node.append_child().save(rhs->material->GetFileName(), ryml::VAL_DQUO);
        node.append_child().save(rhs->submesh, ryml::VAL_PLAIN);
    }
}
