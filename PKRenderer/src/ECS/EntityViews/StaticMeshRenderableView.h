#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/StaticMeshReference.h"
#include "ECS/Components/Materials.h"

namespace PK::ECS::EntityViews
{
    struct StaticMeshRenderableView : public IEntityView
    {
        Components::Transform* transform;
        Components::StaticMeshReference* staticMesh;
        Components::Materials* materials;
    };
}