#pragma once
#include "Core/ECS/IEntityView.h"
#include "Core/ECS/EntityComponentRef.h"
#include "App/ECS/ComponentScenePrimitive.h"
#include "App/ECS/ComponentTransform.h"
#include "App/ECS/ComponentMeshStatic.h"
#include "App/ECS/ComponentMaterials.h"

namespace PK::App
{
    struct EntityViewMeshStatic : public IEntityView
    {
        EntityComponentRef<ComponentScenePrimitive> primitive;
        EntityComponentRef<ComponentTransform> transform;
        EntityComponentRef<ComponentMeshStatic> staticMesh;
        EntityComponentRef<ComponentMaterials> materials;
    };
}
