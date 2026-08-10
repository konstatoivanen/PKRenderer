#pragma once
#include "App/Renderer/EntityEnums.h"
#include "Core/ECS/NotSerialized.h"

namespace PK::App
{
    struct ComponentScenePrimitive
    {
        ScenePrimitiveFlags flags = ScenePrimitiveFlags::Mesh;

        // Runtime state flags
        PK_ECS_PRIVATE_FIELDS_FLAGS
        bool isVisibleInScene = false;
        bool isVisibleInRayTracing = false;
        bool isVisibleLight = false;
        bool isVisibleInShadowmaps = false;
    };
}
