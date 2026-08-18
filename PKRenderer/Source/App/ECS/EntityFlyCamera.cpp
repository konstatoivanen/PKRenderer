#include "PrecompiledHeader.h"
#include "App/ECS/EntityFlyCamera.h"

namespace PK::App
{
    void EntityFlyCamera::OnCreate([[maybe_unused]] EntityDatabase* entityDb, EntityFlyCamera& entity, const Descriptor& desc)
    {
        entity.bounds->localAABB = {};
        entity.transform->position = desc.position;
        entity.transform->rotation = quaternion(desc.rotation);
        entity.transform->scale = PK_FLOAT3_ONE;
        entity.renderView->name = desc.name;
        entity.renderView->desiredRect = desc.desiredRect;
        entity.renderView->isWindowTarget = desc.isWindowTarget;
        entity.renderView->settingsRef = desc.settings;
        entity.projection->mode = App::ComponentProjection::Perspective;
        entity.projection->fieldOfView = desc.fieldOfView;
        entity.projection->zNear = desc.zNear;
        entity.projection->zFar = desc.zFar;
        entity.flyCamera->snapshotPosition = desc.position;
        entity.flyCamera->snapshotRotation = desc.rotation;
        entity.flyCamera->targetPosition = desc.position;
        entity.flyCamera->eulerAngles = desc.rotation;
        entity.flyCamera->moveSpeed = desc.moveSpeed;
        entity.flyCamera->moveSmoothing = desc.moveSmoothing;
        entity.flyCamera->rotationSmoothing = desc.rotationSmoothing;
        entity.flyCamera->sensitivity = desc.sensitivity;
    }
}
