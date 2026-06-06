#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentMeshStatic.h"
#include "App/ECS/ComponentMaterials.h"

namespace PK::App
{
    PK_ECS_VIEW_BEGIN(EntityViewMeshStatic)
        PK_ECS_VIEW_COMPONENT(ComponentScenePrimitive, primitive)
        PK_ECS_VIEW_COMPONENT(ComponentTransform, transform)
        PK_ECS_VIEW_COMPONENT(ComponentMeshStatic, staticMesh)
        PK_ECS_VIEW_COMPONENT(ComponentMaterials, materials)
    PK_ECS_VIEW_END()
}
