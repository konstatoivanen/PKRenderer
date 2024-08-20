#include "PrecompiledHeader.h"
#include <rapidyaml/ryaml.h>
#include "Core/Yaml/RapidyamlFwd.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/TextureAsset.h"

namespace PK::YAML
{
    template<>
    bool Read<TextureAsset*>(const ConstNode& node, TextureAsset** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        *rhs = AssetDatabase::Get()->Load<TextureAsset>(path);
        return *rhs != nullptr;
    }

    template<>
    bool Read<RHITexture*>(const ConstNode& node, RHITexture** rhs)
    {
        auto pathsubstr = node.val();
        FixedString128 path(pathsubstr.len, pathsubstr.data());
        auto asset = AssetDatabase::Get()->Load<TextureAsset>(path);
        *rhs = asset ? asset->GetRHI() : nullptr;
        return *rhs != nullptr;
    }

    PK_YAML_DECLARE_READ_MEMBER(TextureAsset*)
    PK_YAML_DECLARE_READ_MEMBER(RHITexture*)
}