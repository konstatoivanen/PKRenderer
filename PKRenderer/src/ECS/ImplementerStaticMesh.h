#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentBounds.h"
#include "ECS/ComponentScenePrimitive.h"
#include "ECS/ComponentStaticMesh.h"
#include "ECS/ComponentMaterials.h"

namespace PK::ECS
{
    struct ImplementerStaticMesh : public IImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentStaticMesh,
        public ComponentMaterials
    {
    };
}