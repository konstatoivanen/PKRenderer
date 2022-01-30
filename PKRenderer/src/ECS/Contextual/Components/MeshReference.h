#pragma once
#include "Rendering/Objects/Mesh.h"

namespace PK::ECS::Components
{
    struct MeshReference
    {
        PK::Rendering::Objects::Mesh* sharedMesh = nullptr;
        virtual ~MeshReference() = default;
    };
}