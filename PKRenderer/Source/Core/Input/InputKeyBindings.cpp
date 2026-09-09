#include "PrecompiledHeader.h"
#include "InputKeyBindings.h"

namespace PK
{
    bool InputKeyBindings::TryGetKey(const char* command, InputTriplet* outKey) const
    {
        auto valueRef = GetValuePtr(command);

        if (valueRef != nullptr)
        {
            *outKey = *valueRef;
            return true;
        }

        return false;
    }
}
