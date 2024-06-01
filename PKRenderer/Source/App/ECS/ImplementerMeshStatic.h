#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentMeshStatic.h"
#include "App/ECS/ComponentMaterials.h"

namespace PK::App
{
    struct ImplementerMeshStatic : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentMeshStatic,
        public ComponentMaterials
    {
    };
}