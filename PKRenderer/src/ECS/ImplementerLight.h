#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentBounds.h"
#include "ECS/ComponentScenePrimitive.h"
#include "ECS/ComponentLight.h"

namespace PK::ECS
{
    struct ImplementerLight : public IImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentLight
    {
    };
}