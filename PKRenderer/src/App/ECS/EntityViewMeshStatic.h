#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentMeshStatic.h"
#include "App/ECS/ComponentMaterials.h"

namespace PK::App
{
    struct EntityViewMeshStatic : public IEntityView
    {
        ComponentScenePrimitive* primitive;
        ComponentTransform* transform;
        ComponentMeshStatic* staticMesh;
        ComponentMaterials* materials;
    };
}