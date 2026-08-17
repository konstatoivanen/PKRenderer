#pragma once
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"
#include "App/ECS/ComponentFlyCamera.h"

namespace PK::App
{
    struct EntityViewFlyCamera
    {
        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentProjection* projection;
        ComponentViewInput* input;
        ComponentTime* time;
        ComponentFlyCamera* flyCamera;
    };
}
