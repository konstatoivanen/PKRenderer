#pragma once
#include "Core/Utilities/FixedMask.h"
#include "Core/Input/InputState.h"

namespace PK::App
{
    struct ComponentViewInput
    {
        InputState state{};

        // Current index holding input focus.
        uint32_t hotControlId = 0u;

        // Id counter for control id systems. rebuilt every frame.
        uint32_t controlIdCounter = 0u;

        virtual ~ComponentViewInput() = default;
    };
}
