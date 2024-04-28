#include "PrecompiledHeader.h"
#include "InputKeyBinding.h"

namespace PK::Core::Input
{
    void CommandInputKeyBindingMap::TryGetKey(const char* command, InputKey* outKey) const
    {
        auto iter = find(std::string(command));

        if (iter != end())
        {
            *outKey = iter->second;
        }
    }
}
