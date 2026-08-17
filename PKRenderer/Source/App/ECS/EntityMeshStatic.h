#pragma once
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewMeshStatic.h"

namespace PK::App
{
    struct EntityMeshStatic
    {
        struct Descriptor
        {
            FixedString32 entityName;
            bool entitySerialize;

            ScenePrimitiveFlags flags;
            MeshStaticRef mesh;
            BufferView<MaterialTarget> materials;
            float3 position;
            float3 rotation;
            float3 scale;
        };

        uint32_t* entityId;
        ComponentTransform* transform;
        ComponentBounds* bounds;
        ComponentScenePrimitive* primitive;
        ComponentMeshStatic* mesh;
        ComponentMaterials* materials;

        static void OnCreate(EntityDatabase* entityDb, EntityMeshStatic& entity, const Descriptor& descriptor);
    };
}
