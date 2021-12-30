#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Components/Transform.h"
#include "ECS/Contextual/Components/Light.h"
#include "ECS/Contextual/Components/Renderable.h"
#include "ECS/Contextual/Components/Bounds.h"

namespace PK::ECS::EntityViews
{
    struct LightRenderableView : public IEntityView
    {
        Components::Transform* transform;
        Components::Bounds* bounds;
        Components::Light* light;
        Components::Renderable* renderable;
    };
}