#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentBounds.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentLight.h"

namespace PK::App
{
    struct ImplementerLight : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentLight
    {
    };
}