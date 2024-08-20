#include "PrecompiledHeader.h"
#include <rapidyaml/ryaml.h>
#include "Core/Yaml/RapidyamlFwd.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"

namespace PK::YAML
{
    template<>
    bool Read<ShaderAsset*>(const ConstNode& node, ShaderAsset** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<ShaderAsset>(path);
        return *rhs != nullptr;
    }

    PK_YAML_DECLARE_READ_MEMBER(ShaderAsset*)
}
