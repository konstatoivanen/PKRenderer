#pragma once
#include <yaml-cpp/yaml.h>
#include "Core/Input/InputKeyBinding.h"

namespace YAML
{
	template<>
	struct convert<PK::Core::Input::CommandInputKeyBindingMap>
	{
		static Node encode(const PK::Core::Input::CommandInputKeyBindingMap& rhs);
		static bool decode(const Node& node, PK::Core::Input::CommandInputKeyBindingMap& rhs);
	};

	template<>
	struct convert<PK::Core::Input::InputKeyCommandBindingMap>
	{
		static Node encode(const PK::Core::Input::InputKeyCommandBindingMap& rhs);
		static bool decode(const Node& node, PK::Core::Input::InputKeyCommandBindingMap& rhs);
	};
}