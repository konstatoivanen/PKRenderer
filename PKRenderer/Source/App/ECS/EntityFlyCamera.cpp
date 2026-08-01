#include "PrecompiledHeader.h"
#include "Core/ECS/EntityDatabase.h"
#include "App/ECS/EntityViewTransform.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/ECS/EntityFlyCamera.h"

namespace PK::App
{
    void EntityFlyCamera::OnCreate(
        [[maybe_unused]] EntityDatabase* entityDb, 
        [[maybe_unused]] const EGID& egid, 
        const EntityFlyCamera& desc, 
        TImplementers& implementers)
    {
        auto implementer = Sequence::GetAt<0>(implementers);
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
    }
}
