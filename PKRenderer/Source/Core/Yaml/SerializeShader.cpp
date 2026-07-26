#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Yaml/Serialize.h"

namespace PK::YAML
{
    template<>
    void Read<ShaderAsset*>(const ConstNode& node, ShaderAsset** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<ShaderAsset>(path).get();
    }

    template<>
    void Read<ShaderAssetRef>(const ConstNode& node, ShaderAssetRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<ShaderAsset>(path);
    }

    template<>
    void Write<ShaderAsset*>(Node& node, ShaderAsset* const* rhs)
    {
        node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<ShaderAssetRef>(Node& node, const ShaderAssetRef* rhs)
    {
        node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }
}
