#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Bounds.h"
#include "ECS/Components/Renderable.h"
#include "ECS/Components/MeshReference.h"
#include "ECS/Components/Materials.h"

namespace PK::ECS::Implementers
{
    struct MeshRenderableImplementer : public IImplementer,
        public Components::Transform,
        public Components::Bounds,
        public Components::Renderable,
        public Components::MeshReference,
        public Components::Materials
    {
    };
}