#include "PrecompiledHeader.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/Rendering/Window.h"
#include "Core/CLI/Log.h"
#include "App/FrameContext.h"
#include "EngineInput.h"

namespace PK::App
{
    void EngineInput::OnStepFrameUpdate(FrameContext* ctx)
    {
        m_deltaMillis = (uint32_t)(1000.0 * ctx->time.unscaledDeltaTime);
        ctx->input.lastDeviceState.device = m_lastDevice;
        ctx->input.lastDeviceState.state = m_deviceStates.GetValuePtr(m_lastDevice);

        if (m_deviceStates.GetCount() >= InputStateCollection::MAX_DEVICES)
        {
            PK_LOG_WARNING("Active input device count exceeds input context device limit. some device states will be ignored!");
        }

        for (auto i = 0u; i < m_deviceStates.GetCount() && i < InputStateCollection::MAX_DEVICES; ++i)
        {
            auto pair = ctx->input.deviceStates.Add();
            pair->device = m_deviceStates[i].key;
            pair->state = &m_deviceStates[i].value;
        }

        m_sequencer->Next(this, &ctx->input);
    }

    void EngineInput::InputHandler_OnPoll()
    {
        m_droppedFilePaths.device = nullptr;
    }

    void EngineInput::InputHandler_OnPoll(InputDevice* device)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->SwapBuffers(m_deltaMillis);
        }
    }

    void EngineInput::InputHandler_OnKey(InputDevice* device, InputKey key, bool isDown)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state && key != InputKey::None)
        {
            state->SetKey(key, isDown);
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnMouseMoved(InputDevice* device, const float2& position, const float2& size)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->SetCursor(position, size);
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnScroll(InputDevice* device, uint32_t axis, float offset)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->cursorScroll[axis] = offset;
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnCharacter(InputDevice* device, uint32_t character)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->character = character;
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnDrop(InputDevice* device, const char* const* paths, uint32_t count)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            m_droppedFilePaths.device = device;
            m_droppedFilePaths.paths = CArgumentsInlineDefault(paths, count);
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnConnect(InputDevice* device)
    {
        auto index = 0u;

        if (m_deviceStates.AddKey(device, &index))
        {
            m_deviceStates[index].value = {};
        }
    }

    void EngineInput::InputHandler_OnDisconnect(InputDevice* device)
    {
        auto statePtr = m_deviceStates.GetValuePtr(device);

        if (statePtr)
        {
            m_deviceStates.Remove(device);
        }

        if (m_lastDevice == device)
        {
            m_lastDevice = m_deviceStates.GetCount() > 0u ? m_deviceStates[0].key : nullptr;
        }
    }
}
