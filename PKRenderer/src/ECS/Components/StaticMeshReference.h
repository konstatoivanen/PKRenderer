#pragma once
#include "Rendering/Objects/StaticSceneMesh.h"

namespace PK::ECS::Components
{
    struct StaticMeshReference
    {
        PK::Rendering::Objects::StaticMesh* sharedMesh = nullptr;
        virtual ~StaticMeshReference() = default;
    };
}