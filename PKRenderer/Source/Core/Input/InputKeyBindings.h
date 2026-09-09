#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Input/InputKey.h"

namespace PK
{
    struct InputKeyBindings : public HashMap<FixedString32, InputTriplet>
    {
        bool TryGetKey(const char* command, InputTriplet* outKey) const;
    };
}
