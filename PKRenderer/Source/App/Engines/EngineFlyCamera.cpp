#include "PrecompiledHeader.h"
#include "Core/Math/Extended.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/IApplication.h"
#include "Core/Rendering/Window.h"
#include "App/ECS/EntityViewFlyCamera.h"
#include "App/FrameContext.h"
#include "EngineFlyCamera.h"

namespace PK::App
{
    EngineFlyCamera::EngineFlyCamera(EntityDatabase* entityDb, InputKeyConfig* keyConfig) : m_entityDb(entityDb)
    {
        m_keys.SetKeysFrom(keyConfig);
        CVariableRegister::Create<CVariableFuncSimple>("Engine.FlyCamera.Transforms.Log", [this](){TransformsLog();});
        CVariableRegister::Create<CVariableFuncSimple>("Engine.FlyCamera.Transforms.Reset", [this](){TransformsReset();});
    }

    void EngineFlyCamera::OnStepFrameUpdate(FrameContext* ctx)
    {
        auto views = m_entityDb->Query<EntityViewFlyCamera>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            auto& view = views[i];
            auto& input = view.input;
            auto& time = view.time->info;
            auto camera = view.flyCamera;
            auto projection = view.projection;
            auto transform = view.transform;

            auto sensitivity = camera->sensitivity / 1000.0f;
            auto deltaTime = math::clamp((float)time.deltaTime, 0.001f, 0.99f);
            auto interpolantPos = 1.0f - math::exp(-deltaTime / camera->moveSmoothing);
            auto interpolantRot = 1.0f - math::exp(-deltaTime / camera->rotationSmoothing);
            auto isDragging = input->state.ConsumeKey(m_keys.LookDrag);

            if (isDragging)
            {
                camera->eulerAngles.x += input->state.cursorPositionDelta.y * sensitivity;
                camera->eulerAngles.y -= input->state.cursorPositionDelta.x * sensitivity;
            }

            auto targetRotation = quaternion(camera->eulerAngles);

            auto offset = input->state.ConsumeAxis(m_keys.Left, m_keys.Right, m_keys.Down, m_keys.Up, m_keys.Backward, m_keys.Forward);
            auto length = math::length(offset);

            if (length > 0)
            {
                offset /= length;
                offset *= deltaTime * camera->moveSpeed;
            }

            camera->targetPosition += math::mul(targetRotation, offset);

            if (input->state.ConsumeKey(m_keys.FovControl))
            {
                auto fovDelta = input->state.ConsumeAxis(m_keys.FovSub, m_keys.FovAdd);

                if (input->state.ConsumeKey(m_keys.DollyZoom))
                {
                    auto fov0 = projection->fieldOfView;
                    auto fov1 = projection->fieldOfView + fovDelta;
                    auto fd0 = math::cot(fov0 * PK_FLOAT_DEG2RAD * 0.5f);
                    auto fd1 = math::cot(fov1 * PK_FLOAT_DEG2RAD * 0.5f);
                    auto zoomOffset = math::mul(targetRotation, float3(0, 0, fd0 - fd1));

                    camera->targetPosition += zoomOffset;
                    transform->position += zoomOffset;
                }

                projection->fieldOfView += fovDelta;
            }
            else
            {
                auto speedDelta = input->state.ConsumeAxis(m_keys.SpeedDown, m_keys.SpeedUp);
                camera->moveSpeed += camera->moveSpeed * 0.25f * speedDelta;
                camera->moveSpeed = math::max(0.01f, camera->moveSpeed);

                if (input->state.ConsumeKey(m_keys.ResetSmoothing))
                {
                    interpolantPos = 1.0f;
                    interpolantRot = 1.0f;
                }
            }

            transform->position = math::lerp(transform->position, camera->targetPosition, interpolantPos);
            transform->rotation = math::slerp(transform->rotation, targetRotation, interpolantRot);

            // Force automatic perspective projection for this view.
            view.projection->mode = ComponentProjection::Perspective;

            ctx->window->SetCursorLock(isDragging, !isDragging);
            ctx->window->GetNative()->SetUseRawInput(true);
        }
    }

    void EngineFlyCamera::TransformsLog() const
    {
        auto views = m_entityDb->Query<EntityViewFlyCamera>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            auto& view = views[i];
            auto transform = view.transform;
            auto position = transform->position;
            auto rotation = math::euler(transform->rotation);
            PK_LOG_INFO("EngineFlyCamera.Transforms.Log: EntityId:%i Pos:[%f, %f, %f], Rot:[%f,%f,%f]", view.GID.entityID(), position.x, position.y, position.z, rotation.x, rotation.y, rotation.z);
        }
    }

    void EngineFlyCamera::TransformsReset()
    {
        auto views = m_entityDb->Query<EntityViewFlyCamera>((uint32_t)ENTITY_GROUPS::ACTIVE);

        for (auto i = 0u; i < views.count; ++i)
        {
            auto& view = views[i];
            auto camera = view.flyCamera;
            auto transform = view.transform;

            camera->eulerAngles = camera->snapshotRotation;
            camera->targetPosition = transform->position = camera->snapshotPosition;
            transform->rotation = quaternion(camera->eulerAngles);
            PK_LOG_INFO("EngineFlyCamera.Transforms.Reset: EntityId:%i", i);
        }
    }
}
