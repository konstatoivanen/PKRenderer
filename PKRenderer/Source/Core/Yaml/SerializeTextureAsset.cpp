#include "PrecompiledHeader.h"
#include "Core/Yaml/Serialize.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/TextureAsset.h"

namespace PK::Serialize
{
    template<>
    void Read<TextureAsset*>(const ConstNode& node, TextureAsset** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<TextureAsset>(path).get();
    }

    template<>
    void Read<TextureAssetRef>(const ConstNode& node, TextureAssetRef* rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<TextureAsset>(path);
    }

    template<>
    void Read<RHITexture*>(const ConstNode& node, RHITexture** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        auto asset = AssetDatabase::Get()->Load<TextureAsset>(path);
        *rhs = asset ? asset->GetRHI() : nullptr;
    }

    template<>
    void Write<TextureAsset*>(Node& node, TextureAsset* const* rhs)
    {
        node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }

    template<>
    void Write<TextureAssetRef>(Node& node, const TextureAssetRef* rhs)
    {
        node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
    }
}
