#pragma once
#include "Rendering/Structs/Enums.h"
#include "Math/Types.h"

namespace PK::ECS::Components
{
    struct Light
    {
        PK::Math::color color = PK::Math::PK_COLOR_WHITE;
        float radius = 1.0f;
        float angle = 45.0f;
        float shadowBlur = 0.1f;
        PK::Rendering::Structs::Cookie cookie = PK::Rendering::Structs::Cookie::Circle0;
        PK::Rendering::Structs::LightType type = PK::Rendering::Structs::LightType::Point;
        virtual ~Light() = default;
    };

    struct LightFrameInfo
    {
        uint16_t batchGroup = 0u;
        uint16_t shadowmapIndex = 0u;
        uint16_t projectionIndex = 0u;
        float maxShadowDepth = 0.0f;
        float minShadowDepth = 0.0f;
        virtual ~LightFrameInfo() = default;
    };
}