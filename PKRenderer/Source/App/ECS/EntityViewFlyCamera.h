#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentProjection.h"
#include "App/ECS/ComponentViewInput.h"
#include "App/ECS/ComponentTime.h"
#include "App/ECS/ComponentFlyCamera.h"

namespace PK::App
{
    struct EntityViewFlyCamera : public IEntityView
    {
        EntityComponentRef<ComponentTransform> transform;
        EntityComponentRef<ComponentProjection> projection;
        EntityComponentRef<ComponentViewInput> input;
        EntityComponentRef<ComponentTime> time;
        EntityComponentRef<ComponentFlyCamera> flyCamera;
    };
}
