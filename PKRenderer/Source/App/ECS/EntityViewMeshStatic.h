#pragma once
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentMeshStatic.h"
#include "App/ECS/ComponentMaterials.h"

namespace PK::App
{
    struct EntityViewMeshStatic
    {
        uint32_t* entityId;
        ComponentScenePrimitive* primitive;
        ComponentTransform* transform;
        ComponentMeshStatic* staticMesh;
        ComponentMaterials* materials;
    };
}
