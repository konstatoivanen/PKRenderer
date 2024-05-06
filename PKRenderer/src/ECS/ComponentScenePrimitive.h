#pragma once
#include "Rendering/EntityEnums.h"

namespace PK::ECS
{
    struct ComponentScenePrimitive
    {
        PK::Rendering::ScenePrimitiveFlags flags = PK::Rendering::ScenePrimitiveFlags::Mesh;
        bool isVisibleInScene = false;
        bool isVisibleInRayTracing = false;
        bool isVisibleLight = false;
        bool isVisibleInShadowmaps = false;

        virtual ~ComponentScenePrimitive() = default;
    };
}