#include "PrecompiledHeader.h"
#include "Core/Application.h"
#include "Core/Assets/AssetDatabase.h"
#include "ConvertTextureAsset.h"

namespace YAML
{
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Rendering::Objects;

    Node convert<TextureAsset*>::encode(const TextureAsset*& rhs)
    {
        Node node;
        node.push_back(rhs->GetFileName());
        node.SetStyle(EmitterStyle::Default);
        return node;
    }

    bool convert<TextureAsset*>::decode(const Node& node, TextureAsset*& rhs)
    {
        rhs = PK::Core::Application::GetService<AssetDatabase>()->Load<TextureAsset>(node.as<std::string>());
        return true;
    }
}