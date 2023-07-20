#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Renderable.h"
#include "ECS/Components/Bounds.h"

namespace PK::ECS::EntityViews
{
    struct LightRenderableView : public IEntityView
    {
        Components::Transform* transform;
        Components::Bounds* bounds;
        Components::Light* light;
        Components::LightFrameInfo* lightFrameInfo;
        Components::Renderable* renderable;
    };
}