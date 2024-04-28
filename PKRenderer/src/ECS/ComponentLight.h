#pragma once
#include "Math/Types.h"
#include "Rendering/EntityEnums.h"

namespace PK::ECS
{
    struct ComponentLight
    {
        PK::Math::color color = PK::Math::PK_COLOR_WHITE;
        float sourceRadius = 0.1f;
        float radius = 1.0f;
        float angle = 45.0f;
        float shadowBlur = 0.1f;
        PK::Rendering::LightCookie cookie = PK::Rendering::LightCookie::Circle0;
        PK::Rendering::LightType type = PK::Rendering::LightType::Point;
        virtual ~ComponentLight() = default;
    };
}