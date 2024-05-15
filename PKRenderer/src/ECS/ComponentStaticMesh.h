#pragma once
#include "Rendering/Objects/StaticMeshCollection.h"

namespace PK::ECS
{
    struct ComponentStaticMesh
    {
        PK::Rendering::Objects::StaticMesh* sharedMesh = nullptr;
        virtual ~ComponentStaticMesh() = default;
    };
}