#pragma once
#include "Math/Types.h"
#include "Renderer/EntityEnums.h"

namespace PK::ECS
{
    struct ComponentLight
    {
        PK::Math::color color = PK::Math::PK_COLOR_WHITE;
        float sourceRadius = 0.1f;
        float radius = 1.0f;
        float angle = 45.0f;
        float shadowBlur = 0.1f;
        PK::Renderer::LightCookie cookie = PK::Renderer::LightCookie::Circle0;
        PK::Renderer::LightType type = PK::Renderer::LightType::Point;
        virtual ~ComponentLight() = default;
    };
}