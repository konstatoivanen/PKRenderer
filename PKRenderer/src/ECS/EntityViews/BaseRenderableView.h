#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Bounds.h"
#include "ECS/Components/Renderable.h"

namespace PK::ECS::EntityViews
{
    struct BaseRenderableView : public IEntityView
    {
        Components::Bounds* bounds;
        Components::Renderable* renderable;
    };
}