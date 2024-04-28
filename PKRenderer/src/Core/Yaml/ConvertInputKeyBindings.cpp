#include "PrecompiledHeader.h"
#include "ConvertInputKeyBindings.h"

namespace YAML
{
	using namespace PK::Core::Input;

	Node convert<CommandInputKeyBindingMap>::encode(const CommandInputKeyBindingMap& rhs)
	{
		Node node;
		for (auto& binding : rhs)
		{
			Node bindingNode;
			bindingNode.push_back(binding.first);
			bindingNode.push_back(KeyToString(binding.second));
			bindingNode.SetStyle(EmitterStyle::Flow);
			node.push_back(bindingNode);
		}

		node.SetStyle(EmitterStyle::Default);
		return node;
	}

	bool convert<CommandInputKeyBindingMap>::decode(const Node& node, CommandInputKeyBindingMap& rhs)
	{
		for (auto i = 0; i < node.size(); ++i)
		{
			rhs[node[i][0].as<std::string>()] = StringToKey(node[i][1].as<std::string>());
		}

		return true;
	}

	Node convert<InputKeyCommandBindingMap>::encode(const InputKeyCommandBindingMap& rhs)
	{
		Node node;
		for (auto& binding : rhs)
		{
			Node bindingNode;
			bindingNode.push_back(KeyToString(binding.first));
			bindingNode.push_back(binding.second);
			bindingNode.SetStyle(EmitterStyle::Flow);
			node.push_back(bindingNode);
		}

		node.SetStyle(EmitterStyle::Default);
		return node;
	}

	bool convert<InputKeyCommandBindingMap>::decode(const Node& node, InputKeyCommandBindingMap& rhs)
	{
		for (auto i = 0; i < node.size(); ++i)
		{
			rhs[StringToKey(node[i][0].as<std::string>())] = node[i][1].as<std::string>();
		}

		return true;
	}
}