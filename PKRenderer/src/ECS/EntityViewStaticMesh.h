#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentScenePrimitive.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentStaticMesh.h"
#include "ECS/ComponentMaterials.h"

namespace PK::ECS
{
    struct EntityViewStaticMesh : public IEntityView
    {
        ComponentScenePrimitive* primitive;
        ComponentTransform* transform;
        ComponentStaticMesh* staticMesh;
        ComponentMaterials* materials;
    };
}