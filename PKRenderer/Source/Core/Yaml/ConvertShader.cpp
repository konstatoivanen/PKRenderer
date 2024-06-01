#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "ConvertShader.h"

namespace YAML
{
    Node convert<PK::ShaderAsset*>::encode(const PK::ShaderAsset*& rhs)
    {
        Node node;
        node.push_back(rhs->GetFileName());
        node.SetStyle(EmitterStyle::Default);
        return node;
    }

    bool convert<PK::ShaderAsset*>::decode(const Node& node, PK::ShaderAsset*& rhs)
    {
        rhs = PK::AssetDatabase::Get()->Load<PK::ShaderAsset>(node.as<std::string>());
        return true;
    }
}