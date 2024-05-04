#include "PrecompiledHeader.h"
#include "Core/Application.h"
#include "Core/Assets/AssetDatabase.h"
#include "ConvertTexture.h"

namespace YAML
{
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Rendering::RHI::Objects;

    Node convert<Texture*>::encode(const Texture*& rhs)
    {
        Node node;
        node.push_back(rhs->GetFileName());
        node.SetStyle(EmitterStyle::Default);
        return node;
    }

    bool convert<Texture*>::decode(const Node& node, Texture*& rhs)
    {
        rhs = PK::Core::Application::GetService<AssetDatabase>()->Load<Texture>(node.as<std::string>());
        return true;
    }
}