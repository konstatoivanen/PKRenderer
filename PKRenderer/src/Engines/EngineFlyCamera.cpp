#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsMatrix.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Input/InputDevice.h"
#include "ECS/EntityViewFlyCamera.h"
#include "EngineFlyCamera.h"

namespace PK::Engines
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Core::CLI;
    using namespace PK::Core::Input;
    using namespace PK::ECS;

    EngineFlyCamera::EngineFlyCamera(EntityDatabase* entityDb, Core::Input::InputKeyConfig* keyConfig) : m_entityDb(entityDb)
    {
        m_keys.SetKeysFrom(keyConfig);
        CVariableRegister::Create<CVariableFunc>("Engine.FlyCamera.Transforms.Log", [this](const char** args, uint32_t count) { TransformsLog(); });
        CVariableRegister::Create<CVariableFunc>("Engine.FlyCamera.Transforms.Reset", [this](const char** args, uint32_t count) { TransformsReset(); });
    }

    void EngineFlyCamera::Step(InputDevice* input)
    {
        auto views = m_entityDb->Query<EntityViewFlyCamera>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < views.count; ++i)
        {
            auto& view = views[i];
            auto camera = view.flyCamera;
            auto projection = view.projection;
            auto transform = view.transform;

            auto sensitivity = camera->sensitivity / 1000.0f;
            auto deltaTime = glm::clamp((float)m_timeFrameInfo.deltaTime, 0.001f, 0.99f);
            auto interpolantPos = glm::clamp(deltaTime / camera->moveSmoothing, 0.0f, 1.0f);
            auto interpolantRot = glm::clamp(deltaTime / camera->rotationSmoothing, 0.0f, 1.0f);

            if (input->GetKey(m_keys.LookDrag))
            {
                camera->eulerAngles.x += input->GetCursorDeltaY() * sensitivity;
                camera->eulerAngles.y -= input->GetCursorDeltaX() * sensitivity;
            }

            auto offset = input->GetAxis(m_keys.Left, m_keys.Right, m_keys.Down, m_keys.Up, m_keys.Backward, m_keys.Forward);
            auto length = glm::length(offset);

            if (length > 0)
            {
                offset /= length;
                offset *= deltaTime * camera->moveSpeed;
            }

            auto targetRotation = glm::quat(camera->eulerAngles);
            camera->targetPosition += targetRotation * offset;


            if (input->GetKey(m_keys.FovControl))
            {
                auto fovDelta = input->GetAxis(m_keys.FovSub, m_keys.FovAdd);

                if (input->GetKey(m_keys.DollyZoom))
                {
                    auto fov0 = projection->fieldOfView;
                    auto fov1 = projection->fieldOfView + fovDelta;
                    auto fd0 = Functions::Cot(fov0 * PK_FLOAT_DEG2RAD * 0.5f);
                    auto fd1 = Functions::Cot(fov1 * PK_FLOAT_DEG2RAD * 0.5f);
                    auto zoomOffset = targetRotation * float3(0, 0, fd0 - fd1);

                    camera->targetPosition += zoomOffset;
                    transform->position += zoomOffset;
                }

                projection->fieldOfView += fovDelta;
            }
            else
            {
                auto speedDelta = input->GetAxis(m_keys.SpeedDown, m_keys.SpeedUp);
                camera->moveSpeed += camera->moveSpeed * 0.25f * speedDelta;
                camera->moveSpeed = glm::max(0.01f, camera->moveSpeed);

                if (input->GetKey(m_keys.ResetSmoothing))
                {
                    interpolantPos = 1.0f;
                    interpolantRot = 1.0f;
                }
            }

            transform->position = glm::mix(transform->position, camera->targetPosition, interpolantPos);
            transform->rotation = glm::slerp(transform->rotation, targetRotation, interpolantRot);

            // Force automatic perspective projection for this view.
            view.projection->mode = ComponentProjection::Perspective;
        }
    }

    void EngineFlyCamera::TransformsLog() const
    {
        auto views = m_entityDb->Query<EntityViewFlyCamera>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < views.count; ++i)
        {
            auto& view = views[i];
            auto transform = view.transform;
            auto position = transform->position;
            auto rotation = glm::eulerAngles(transform->rotation);
            PK_LOG_INFO("EngineFlyCamera.Transforms.Log: EntityId:%i Pos:[%f, %f, %f], Rot:[%f,%f,%f]", view.GID.entityID(), position.x, position.y, position.z, rotation.x, rotation.y, rotation.z);
        }
    }

    void EngineFlyCamera::TransformsReset()
    {
        auto views = m_entityDb->Query<EntityViewFlyCamera>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

        for (auto i = 0; i < views.count; ++i)
        {
            auto& view = views[i];
            auto camera = view.flyCamera;
            auto transform = view.transform;

            camera->eulerAngles = camera->snashotRotation;
            camera->targetPosition = transform->position = camera->snashotPosition;
            transform->rotation = glm::quat(camera->eulerAngles);
            PK_LOG_INFO("EngineFlyCamera.Transforms.Reset: EntityId:%i", i);
        }
    }
}