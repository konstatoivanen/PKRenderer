#pragma once
#include "Core/Rendering/MeshStaticCollection.h"

namespace PK::App
{
    struct ComponentMeshStatic
    {
        MeshStaticRef sharedMesh = nullptr;
        virtual ~ComponentMeshStatic() = default;
    };
}
