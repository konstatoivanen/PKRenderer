#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentScenePrimitive.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentMeshStatic.h"
#include "ECS/ComponentMaterials.h"

namespace PK::ECS
{
    struct EntityViewMeshStatic : public IEntityView
    {
        ComponentScenePrimitive* primitive;
        ComponentTransform* transform;
        ComponentMeshStatic* staticMesh;
        ComponentMaterials* materials;
    };
}