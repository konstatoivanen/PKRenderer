#pragma once
#include "Rendering/Structs/Enums.h"
#include "Math/Types.h"

namespace PK::ECS::Components
{
    using namespace PK::Math;
    using namespace PK::Rendering::Structs;

    struct Light
    {
        color color = PK_COLOR_WHITE;
        float radius = 1.0f;
        float angle = 45.0f;
        float shadowBlur = 0.1f;
        Cookie cookie = Cookie::Circle0;
        LightType type = LightType::Point;
        virtual ~Light() = default;
    };

    struct LightFrameInfo
    {
        uint16_t batchGroup = 0u;
        uint16_t shadowmapIndex = 0u;
        uint16_t projectionIndex = 0u;
        float shadowDepth = 0.0f;
        virtual ~LightFrameInfo() = default;
    };
}