#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Components/Transform.h"
#include "ECS/Contextual/Components/MeshReference.h"
#include "ECS/Contextual/Components/Materials.h"

namespace PK::ECS::EntityViews
{
    struct MeshRenderableView : public IEntityView
    {
        Components::Transform* transform;
        Components::MeshReference* mesh;
        Components::Materials* materials;
    };
}