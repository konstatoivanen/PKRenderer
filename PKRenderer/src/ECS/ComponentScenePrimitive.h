#pragma once
#include "Renderer/EntityEnums.h"

namespace PK::ECS
{
    struct ComponentScenePrimitive
    {
        PK::Renderer::ScenePrimitiveFlags flags = PK::Renderer::ScenePrimitiveFlags::Mesh;
        bool isVisibleInScene = false;
        bool isVisibleInRayTracing = false;
        bool isVisibleLight = false;
        bool isVisibleInShadowmaps = false;

        virtual ~ComponentScenePrimitive() = default;
    };
}