#pragma once
#include "Core/Math/Math.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct ComponentLight
    {
        color color = PK_COLOR_WHITE;
        float sourceRadius = 0.1f;
        float nearClip = 0.1f;
        float exponent = 8.0f;
        float radius = 1.0f;
        float angle = 45.0f;
        float angleFade = 0.01f;
        LightType type = LightType::Point;
        LightCookie cookie = LightCookie::Circle0;
        virtual ~ComponentLight() = default;
    };
}
