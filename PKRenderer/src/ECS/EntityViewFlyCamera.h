#pragma once
#include "ECS/EntityDatabase.h"
#include "ECS/ComponentTransform.h"
#include "ECS/ComponentProjection.h"
#include "ECS/ComponentFlyCamera.h"

namespace PK::ECS
{
    struct EntityViewFlyCamera : public IEntityView
    {
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentFlyCamera* flyCamera;
    };
}