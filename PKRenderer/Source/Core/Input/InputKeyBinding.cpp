#include "PrecompiledHeader.h"
#include "InputKeyBinding.h"

namespace PK
{
    void CommandInputKeyBindingMap::TryGetKey(const char* command, InputKey* outKey) const
    {
        auto valueRef = GetValuePtr(command);

        if (valueRef != nullptr)
        {
            *outKey = *valueRef;
        }
    }
}
