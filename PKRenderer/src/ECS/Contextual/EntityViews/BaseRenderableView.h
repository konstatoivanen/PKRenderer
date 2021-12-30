#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Components/Bounds.h"
#include "ECS/Contextual/Components/Renderable.h"

namespace PK::ECS::EntityViews
{
    struct BaseRenderableView : public IEntityView
    {
        Components::Bounds* bounds;
        Components::Renderable* renderable;
    };
}