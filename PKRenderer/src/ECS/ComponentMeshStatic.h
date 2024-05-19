#pragma once
#include "Graphics/MeshStaticCollection.h"

namespace PK::ECS
{
    struct ComponentMeshStatic
    {
        PK::Graphics::MeshStatic* sharedMesh = nullptr;
        virtual ~ComponentMeshStatic() = default;
    };
}