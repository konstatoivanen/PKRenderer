#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "ConvertCVariableCollection.h"

namespace YAML
{
    Node convert<CVariableCollection>::encode([[maybe_unused]] const CVariableCollection& rhs)
    {
        PK_LOG_WARNING("Warning CVariableCollection encoding is not supported!");
        return {};
    }

    bool convert<CVariableCollection>::decode(const Node& node, [[maybe_unused]] CVariableCollection& rhs)
    {
        std::string argumentsRef[32];
        const char* arguments[32];

        for (auto& child : node)
        {
            auto count = 0u;
            argumentsRef[0] = child.first.as<std::string>();

            if (child.second.IsSequence())
            {
                count = 1u + child.second.size();

                for (auto i = 1u; i < count; ++i)
                {
                    argumentsRef[i] = child.second[i - 1u].as<std::string>();
                }
            }
            else if (child.second.IsScalar())
            {
                count = 2u;
                argumentsRef[1] = child.second.as<std::string>();
            }

            for (auto i = 0u; i < count; ++i)
            {
                arguments[i] = argumentsRef[i].c_str();
            }

            PK::CVariableRegister::Execute(arguments, count);
        }

        return true;
    }
}