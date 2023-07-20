#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Bounds.h"

namespace PK::ECS::EntityViews
{
    struct TransformView : public IEntityView
    {
        Components::Transform* transform;
        Components::Bounds* bounds;
    };
}