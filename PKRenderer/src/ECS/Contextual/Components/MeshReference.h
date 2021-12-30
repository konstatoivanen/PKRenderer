#pragma once
#include "Rendering/Objects/Mesh.h"

namespace PK::ECS::Components
{
    using namespace PK::Rendering::Objects;

    struct MeshReference
    {
        Mesh* sharedMesh = nullptr;
        virtual ~MeshReference() = default;
    };
}