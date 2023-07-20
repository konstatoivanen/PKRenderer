#include "PrecompiledHeader.h"
#include "EngineEditorCamera.h"
#include "Core/Application.h"
#include "Rendering/GraphicsAPI.h"
#include "ECS/Tokens/ViewProjectionToken.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsMatrix.h"

namespace PK::ECS::Engines
{
    using namespace Math;
    using namespace Core;
    using namespace Core::Services;

    EngineEditorCamera::EngineEditorCamera(Sequencer* sequencer, Time* time, const ApplicationConfig* config)
    {
        m_sequencer = sequencer;
        m_config = config;
        m_time = time;
        m_moveSpeed = config->CameraSpeed;
        m_fieldOfView = config->CameraFov;
        m_zNear = config->CameraZNear;
        m_zFar = config->CameraZFar;
        m_moveSmoothing = glm::clamp(config->CameraMoveSmoothing.value, 0.0f, 1.0f);
        m_rotationSmoothing = glm::clamp(config->CameraLookSmoothing.value, 0.0f, 1.0f);
        m_sensitivity = config->CameraLookSensitivity / 1000.0f;

        m_eulerAngles = config->CameraStartRotation.value;
        m_position = m_smoothPosition = config->CameraStartPosition.value;
        m_rotation = m_smoothRotation = glm::quat(m_eulerAngles);
    }

    void EngineEditorCamera::Step(Input* input)
    {
        auto deltaTime = m_time->GetDeltaTime();
        deltaTime = glm::max(0.001f, deltaTime);

        if (input->GetKey(KeyCode::MOUSE1))
        {
            m_eulerAngles.x += input->GetMouseDeltaY() * m_sensitivity;
            m_eulerAngles.y -= input->GetMouseDeltaX() * m_sensitivity;
        }

        auto speed = input->GetKey(KeyCode::LEFT_CONTROL) ? (m_moveSpeed * 0.25f) : input->GetKey(KeyCode::LEFT_SHIFT) ? (m_moveSpeed * 5) : m_moveSpeed;
        auto offset = input->GetAxis3D(KeyCode::Q, KeyCode::E, KeyCode::W, KeyCode::S, KeyCode::D, KeyCode::A);
        auto length = glm::length(offset);

        if (length > 0)
        {
            offset /= length;
            offset *= deltaTime * speed;
        }

        m_rotation = glm::quat(m_eulerAngles);
        m_position += m_rotation * offset;

        auto fdelta = input->GetMouseScrollY() * deltaTime * 1000.0f;

        if (input->GetKey(KeyCode::LEFT_SHIFT))
        {
            auto fov0 = m_fieldOfView;
            auto fov1 = m_fieldOfView - fdelta;
            auto fd0 = Functions::Cot(fov0 * PK_FLOAT_DEG2RAD * 0.5f);
            auto fd1 = Functions::Cot(fov1 * PK_FLOAT_DEG2RAD * 0.5f);
            auto zoomOffset = m_rotation * float3(0, 0, fd0 - fd1);

            m_position += zoomOffset;
            m_smoothPosition += zoomOffset;
        }

        m_fieldOfView -= fdelta;

        m_smoothPosition = glm::mix(m_position, m_smoothPosition, m_moveSmoothing * (1.0f - deltaTime));
        m_smoothRotation = glm::slerp(m_rotation, m_smoothRotation, m_rotationSmoothing * (1.0f - deltaTime));

        Tokens::ViewProjectionUpdateToken token;
        token.projection = Functions::GetPerspective(m_fieldOfView, Application::GetPrimaryWindow()->GetAspectRatioAligned(), m_zNear, m_zFar);
        token.view = Functions::GetMatrixInvTRS(m_smoothPosition, m_smoothRotation, PK_FLOAT3_ONE);
        token.jitter = Math::PK_FLOAT4_ZERO;
        m_sequencer->Next<Tokens::ViewProjectionUpdateToken>(this, &token, 0);
    }

    void EngineEditorCamera::Step(TokenConsoleCommand* token)
    {
        if (!token->isConsumed && token->argument == "log_camera_transform")
        {
            token->isConsumed = true;
            PK_LOG_INFO("Camera Pos: [%f, %f, %f], Rot: [%f,%f,%f]", m_position.x, m_position.y, m_position.z, m_eulerAngles.x, m_eulerAngles.y, m_eulerAngles.z);
            return;
        }

        if (!token->isConsumed && token->argument == "reset_camera_transform")
        {
            token->isConsumed = true;
            m_eulerAngles = m_config->CameraStartRotation.value;
            m_position = m_smoothPosition = m_config->CameraStartPosition.value;
            m_rotation = m_smoothRotation = glm::quat(m_eulerAngles);
            PK_LOG_INFO("Camera Transform Reset!");
            return;
        }
    }
}