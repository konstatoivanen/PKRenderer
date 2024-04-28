#pragma once
#include "Rendering/EntityEnums.h"

namespace PK::ECS
{
    struct ComponentScenePrimitive
    {
        PK::Rendering::ScenePrimitiveFlags flags = PK::Rendering::ScenePrimitiveFlags::Mesh;
        virtual ~ComponentScenePrimitive() = default;
    };
}