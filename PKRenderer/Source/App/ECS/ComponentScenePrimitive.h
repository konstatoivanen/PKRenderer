#pragma once
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    struct ComponentScenePrimitive
    {
        ScenePrimitiveFlags flags = ScenePrimitiveFlags::Mesh;

        // Runtime state flags
        bool isVisibleInScene = false;
        bool isVisibleInRayTracing = false;
        bool isVisibleLight = false;
        bool isVisibleInShadowmaps = false;
    };
}
