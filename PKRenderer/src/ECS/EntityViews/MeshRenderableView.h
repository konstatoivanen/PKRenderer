#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/MeshReference.h"
#include "ECS/Components/Materials.h"

namespace PK::ECS::EntityViews
{
    struct MeshRenderableView : public IEntityView
    {
        Components::Transform* transform;
        Components::MeshReference* mesh;
        Components::Materials* materials;
    };
}