#pragma once
#include "Core/Rendering/Mesh.h"

namespace PK::App
{
    struct ComponentMeshStatic
    {
        MeshStaticRef sharedMesh = nullptr;
        virtual ~ComponentMeshStatic() = default;
    };
}
