#pragma once
#include "Core/ECS/IEntityView.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"
#include "App/ECS/ComponentFlyCamera.h"

namespace PK::App
{
    struct EntityViewFlyCamera : public IEntityView
    {
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentViewInput* input;
        ComponentTime* time;
        ComponentFlyCamera* flyCamera;
    };
}
