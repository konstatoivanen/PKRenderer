#pragma once
#include "Rendering/Structs/Enums.h"
#include "Math/Types.h"

namespace PK::ECS::Components
{
    using namespace PK::Math;
    using namespace PK::Rendering::Structs;

    struct Light
    {
        uint32_t batchGroup = 0u;

        color color = PK_COLOR_WHITE;
        float radius = 1.0f;
        float angle = 45.0f;
        Cookie cookie = Cookie::Circle0;
        LightType type = LightType::Point;
        virtual ~Light() = default;
    };
}