#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Components/Transform.h"
#include "ECS/Contextual/Components/Bounds.h"

namespace PK::ECS::EntityViews
{
    struct TransformView : public IEntityView
    {
        Components::Transform* transform;
        Components::Bounds* bounds;
    };
}