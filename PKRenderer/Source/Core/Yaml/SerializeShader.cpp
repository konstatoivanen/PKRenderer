#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    template<>
    bool Read<ShaderAsset*>(const ConstNode& node, ShaderAsset** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<ShaderAsset>(path).get();
        return *rhs != nullptr;
    }

    template<>
    bool Read<ShaderAssetRef>(const ConstNode& node, ShaderAssetRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<ShaderAsset>(path);
        return *rhs != nullptr;
    }

    PK_YAML_DECLARE_READ_MEMBER(ShaderAsset*)
    PK_YAML_DECLARE_READ_MEMBER(ShaderAssetRef)
}
