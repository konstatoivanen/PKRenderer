#pragma once
#include "Core/Math/Math.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct ComponentLight
    {
        color color = PK_COLOR_WHITE;
        float sourceRadius = 0.1f;
        float radius = 1.0f;
        float angle = 45.0f;
        float shadowBlur = 0.1f;
        LightCookie cookie = LightCookie::Circle0;
        LightType type = LightType::Point;
        virtual ~ComponentLight() = default;
    };
}
