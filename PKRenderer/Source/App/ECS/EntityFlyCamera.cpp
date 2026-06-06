#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/ECS/EntityFlyCamera.h"

namespace PK
{
    template<>
    EGID EntityFactory<App::EntityFlyCamera>::Create(EntityDatabase* entityDb, EGID egid, const App::EntityFlyCamera& desc)
    {
        if (egid.entityID() == 0u)
        {
            egid = entityDb->ReserveEntityId(egid.groupID());
        }
        
        auto implementer = entityDb->NewImplementer<App::ImplementerFlyCamera>();
        entityDb->NewView<App::EntityViewTransform>(implementer, egid);
        entityDb->NewView<App::EntityViewRenderView>(implementer, egid);
        entityDb->NewView<App::EntityViewFlyCamera>(implementer, egid);
        implementer->localAABB = {};
        implementer->position = desc.position;
        implementer->rotation = quaternion(desc.rotation);
        implementer->scale = PK_FLOAT3_ONE;
        implementer->name = desc.name;
        implementer->desiredRect = desc.desiredRect;
        implementer->isWindowTarget = desc.isWindowTarget;
        implementer->mode = App::ComponentProjection::Perspective;
        implementer->snapshotPosition = implementer->position;
        implementer->snapshotRotation = math::euler(implementer->rotation);
        implementer->targetPosition = implementer->snapshotPosition;
        implementer->eulerAngles = implementer->snapshotRotation;
        implementer->fieldOfView = desc.fieldOfView;
        implementer->zNear = desc.zNear;
        implementer->zFar = desc.zFar;
        implementer->moveSpeed = desc.moveSpeed;
        implementer->moveSmoothing = desc.moveSmoothing;
        implementer->rotationSmoothing = desc.rotationSmoothing;
        implementer->sensitivity = desc.sensitivity;
        implementer->settingsRef = desc.settings;
        return egid;
    }

    template<>
    EGID EntityFactory<App::EntityFlyCamera>::CreateDefault(EntityDatabase* entityDb, EGID egid)
    {
        return EGIDInvalid;
    }

    template<>
    EGID EntityFactory<App::EntityFlyCamera>::Deserialize(EntityDatabase* entityDb, const YAML::ConstNode& parent, uint32_t group)
    {
        return EGIDInvalid;
    }

    template<>
    void EntityFactory<App::EntityFlyCamera>::Serialize(EntityDatabase* entityDb, YAML::Node& parent, const EGID& egid)
    {
    }
}
