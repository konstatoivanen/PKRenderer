#pragma once
#include "Core/Utilities/Bitmask.h"
#include "Core/Input/InputState.h"

namespace PK::App
{
    struct ComponentViewInput
    {
        InputState state{};

        // For systems that can have conflicting inputs
        Bitmask<(size_t)InputKey::Count> keysConsumed;
        
        // Current index holding input focus.
        uint32_t hotControlId = 0u;

        // Id counter for control id systems. rebuilt every frame.
        uint32_t controlIdCounter = 0u;

        virtual ~ComponentViewInput() = default;
    };
}