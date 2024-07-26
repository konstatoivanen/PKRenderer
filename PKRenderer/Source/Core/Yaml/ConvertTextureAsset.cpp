#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/TextureAsset.h"
#include "ConvertTextureAsset.h"

namespace YAML
{
    Node convert<PK::TextureAsset*>::encode(const PK::TextureAsset*& rhs)
    {
        Node node;
        node.push_back(rhs->GetFileName());
        node.SetStyle(EmitterStyle::Default);
        return node;
    }

    bool convert<PK::TextureAsset*>::decode(const Node& node, PK::TextureAsset*& rhs)
    {
        rhs = PK::AssetDatabase::Get()->Load<PK::TextureAsset>(node.as<std::string>());
        return true;
    }

    Node convert<PK::RHITexture*>::encode([[maybe_unused]] const PK::RHITexture*& rhs)
    {
        PK_LOG_WARNING("Warning: RHITexture serialization is not supported!");
        return {};
    }

    bool convert<PK::RHITexture*>::decode(const Node& node, PK::RHITexture*& rhs)
    {
        rhs = PK::AssetDatabase::Get()->Load<PK::TextureAsset>(node.as<std::string>())->GetRHI();
        return true;
    }
}