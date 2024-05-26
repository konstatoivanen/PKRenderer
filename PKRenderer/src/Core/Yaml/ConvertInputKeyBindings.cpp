#include "PrecompiledHeader.h"
#include "ConvertInputKeyBindings.h"

namespace YAML
{
    Node convert<PK::CommandInputKeyBindingMap>::encode(const PK::CommandInputKeyBindingMap& rhs)
    {
        Node node;
        for (auto& binding : rhs)
        {
            Node bindingNode;
            bindingNode.push_back(binding.first);
            bindingNode.push_back(std::string(PK::InputKeyToString(binding.second)));
            bindingNode.SetStyle(EmitterStyle::Flow);
            node.push_back(bindingNode);
        }

        node.SetStyle(EmitterStyle::Default);
        return node;
    }

    bool convert<PK::CommandInputKeyBindingMap>::decode(const Node& node, PK::CommandInputKeyBindingMap& rhs)
    {
        for (auto i = 0; i < node.size(); ++i)
        {
            rhs[node[i][0].as<std::string>()] = PK::StringToInputKey(node[i][1].as<std::string>().c_str());
        }

        return true;
    }

    Node convert<PK::InputKeyCommandBindingMap>::encode(const PK::InputKeyCommandBindingMap& rhs)
    {
        Node node;
        for (auto& binding : rhs)
        {
            Node bindingNode;
            bindingNode.push_back(std::string(PK::InputKeyToString(binding.first)));
            bindingNode.push_back(binding.second);
            bindingNode.SetStyle(EmitterStyle::Flow);
            node.push_back(bindingNode);
        }

        node.SetStyle(EmitterStyle::Default);
        return node;
    }

    bool convert<PK::InputKeyCommandBindingMap>::decode(const Node& node, PK::InputKeyCommandBindingMap& rhs)
    {
        for (auto i = 0; i < node.size(); ++i)
        {
            rhs[PK::StringToInputKey(node[i][0].as<std::string>().c_str())] = node[i][1].as<std::string>();
        }

        return true;
    }
}