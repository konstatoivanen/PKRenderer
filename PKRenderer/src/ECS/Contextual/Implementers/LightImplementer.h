#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Components/Transform.h"
#include "ECS/Contextual/Components/Bounds.h"
#include "ECS/Contextual/Components/Renderable.h"
#include "ECS/Contextual/Components/Light.h"

namespace PK::ECS::Implementers
{
    struct LightImplementer : public IImplementer,
        public Components::Transform,
        public Components::Bounds,
        public Components::Renderable,
        public Components::Light,
        public Components::LightFrameInfo
    {
    };
}