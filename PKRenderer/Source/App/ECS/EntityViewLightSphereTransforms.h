#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentTransform.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewLightSphereTransforms)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transformMesh)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transformLight)
    PK_ECS_VIEW_END()
}
