#pragma once
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct ComponentLight
    {
        IESProfileRef IESProfile = nullptr;
        color color = PK_COLOR_WHITE;
        float sourceRadius = 0.1f;
        float nearClip = 0.1f;
        float exponent = 8.0f;
        float radius = 1.0f;
        float angle = 45.0f;
        float angleFade = 0.01f;
        LightType type = LightType::Point;
        bool useIESCandelas = false;
        virtual ~ComponentLight() = default;
    };
}
