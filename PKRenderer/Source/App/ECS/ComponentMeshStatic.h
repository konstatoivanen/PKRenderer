#pragma once
#include "Core/Rendering/MeshStaticCollection.h"

namespace PK::App
{
    struct ComponentMeshStatic
    {
        MeshStatic* sharedMesh = nullptr;
        virtual ~ComponentMeshStatic() = default;
    };
}
