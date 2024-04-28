#pragma once
#include <yaml-cpp/yaml.h>
#include "Rendering/RHI/Objects/Shader.h"

namespace YAML
{
	template<>
	struct convert<PK::Rendering::RHI::Objects::Shader*>
	{
		static Node encode(const PK::Rendering::RHI::Objects::Shader*& rhs);
		static bool decode(const Node& node, PK::Rendering::RHI::Objects::Shader*& rhs);
	};
}