#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/Contextual/Components/Transform.h"
#include "ECS/Contextual/Components/Bounds.h"
#include "ECS/Contextual/Components/Renderable.h"
#include "ECS/Contextual/Components/MeshReference.h"
#include "ECS/Contextual/Components/Materials.h"

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