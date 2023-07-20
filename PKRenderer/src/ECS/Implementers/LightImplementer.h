#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Bounds.h"
#include "ECS/Components/Renderable.h"
#include "ECS/Components/Light.h"

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