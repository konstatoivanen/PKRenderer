#include "PrecompiledHeader.h"
#include "Core/Application.h"
#include "Core/Assets/AssetDatabase.h"
#include "ConvertShader.h"

namespace YAML
{
	using namespace PK::Core;
	using namespace PK::Core::Assets;
	using namespace PK::Rendering::RHI::Objects;

	Node convert<Shader*>::encode(const Shader*& rhs)
	{
		Node node;
		node.push_back(rhs->GetFileName());
		node.SetStyle(EmitterStyle::Default);
		return node;
	}

	bool convert<Shader*>::decode(const Node& node, Shader*& rhs)
	{
		rhs = PK::Core::Application::GetService<AssetDatabase>()->Load<Shader>(node.as<std::string>());
		return true;
	}
}