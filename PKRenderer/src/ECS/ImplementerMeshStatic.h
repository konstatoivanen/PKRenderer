#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentBounds.h"
#include "ECS/ComponentScenePrimitive.h"
#include "ECS/ComponentMeshStatic.h"
#include "ECS/ComponentMaterials.h"

namespace PK::ECS
{
    struct ImplementerMeshStatic : public IImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentMeshStatic,
        public ComponentMaterials
    {
    };
}