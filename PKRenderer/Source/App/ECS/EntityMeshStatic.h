#pragma once
#include "Core/ECS/IEntityImplementer.h"
#include "Core/ECS/EntityFactory.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewScenePrimitive.h"
#include "App/ECS/EntityViewMeshStatic.h"

namespace PK::App
{
    struct ImplementerMeshStatic : public IEntityImplementer,
        public ComponentTransform,
        public ComponentBounds,
        public ComponentScenePrimitive,
        public ComponentMeshStatic,
        public ComponentMaterials
    {
    };

    struct EntityMeshStatic
    {
        using TImplementers = Tuple<ImplementerMeshStatic*>;
        using TViews = Tuple<EntityViewTransform, EntityViewScenePrimitive, EntityViewMeshStatic>;
        constexpr static const bool IsSerializable = false;

        //static void OnDeserialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeRead node, TImplementers& implementers) = delete;
        //static void OnSerialize(EntityDatabase* entityDb, const EGID& egid, SerialNodeWrite node) = delete;
        static void OnCreate(EntityDatabase* entityDb, const EGID& egid, const EntityMeshStatic& descriptor, TImplementers& implementers);

        ScenePrimitiveFlags flags;
        MeshStaticRef mesh;
        BufferView<MaterialTarget> materials;
        float3 position;
        float3 rotation;
        float3 scale;
    };
}
