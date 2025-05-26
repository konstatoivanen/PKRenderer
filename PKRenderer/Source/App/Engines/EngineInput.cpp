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
        ctx->input.globalState = m_globalState;
        ctx->input.lastDeviceState.device = m_lastDevice;
        ctx->input.lastDeviceState.state = {};
        auto pLastState = m_deviceStates.GetValuePtr(m_lastDevice);

        if (pLastState)
        {
            ctx->input.lastDeviceState.state = *pLastState;
        }

        if (m_deviceStates.GetCount() >= InputStateCollection::MAX_DEVICES)
        {
            PK_LOG_WARNING("Active input device count exceeds input context device limit. some device states will be ignored!");
        }

        for (auto i = 0u; i < m_deviceStates.GetCount() && i < InputStateCollection::MAX_DEVICES; ++i)
        {
            auto pair = ctx->input.deviceStates.Add();
            pair->device = m_deviceStates.GetKeyAt(i);
            pair->state = m_deviceStates.GetValueAt(i);
        }

        m_sequencer->Next(this, &ctx->input);
    }

    void EngineInput::InputHandler_OnPoll()
    {
        m_globalState.cursorPositionDelta = PK_FLOAT2_ZERO;
        m_globalState.keysPrevious = m_globalState.keysCurrent;
    }

    void EngineInput::InputHandler_OnPoll(InputDevice* device)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->cursorPositionDelta = PK_FLOAT2_ZERO;
            state->keysPrevious = state->keysCurrent;
        }
    }

    void EngineInput::InputHandler_OnKey(InputDevice* device, InputKey key, bool isDown)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state && key != InputKey::None)
        {
            state->keysCurrent[(uint32_t)key] = isDown;
            m_globalState.keysCurrent[(uint32_t)key] = isDown;
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnMouseMoved(InputDevice* device, const float2& position, const float2& size)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            auto cursorPosition = float2(position.x, size.y - position.y);
            auto cursorPositionNormalized = cursorPosition / size;
            auto cursorPositionDelta = cursorPosition - state->cursorPosition;
            state->cursorPosition = cursorPosition;
            state->cursorPositionDelta += cursorPositionDelta;
            state->cursorPositionNormalized = cursorPositionNormalized;
            m_globalState.cursorPosition = cursorPosition;
            m_globalState.cursorPositionDelta += cursorPositionDelta;
            m_globalState.cursorPositionNormalized = cursorPositionNormalized;
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnScroll(InputDevice* device, uint32_t axis, float offset)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->cursorScroll[axis] = offset;
            m_globalState.cursorScroll[axis] = offset;
            m_lastDevice = device;
        }
    }

    void EngineInput::InputHandler_OnCharacter([[maybe_unused]] InputDevice* device, [[maybe_unused]] uint32_t character)
    {
        //@TODO Do something?!?
        m_lastDevice = device;
    }

    void EngineInput::InputHandler_OnDrop([[maybe_unused]] InputDevice* device, [[maybe_unused]] const char* const* paths, [[maybe_unused]] uint32_t count)
    {
        //@TODO Do something?!?
        m_lastDevice = device;
    }

    void EngineInput::InputHandler_OnConnect(InputDevice* device)
    {
        auto index = 0u;
        if (m_deviceStates.AddKey(device, &index))
        {
            m_deviceStates.SetValueAt(index, {});
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
            m_lastDevice = m_deviceStates.GetCount() > 0u ? m_deviceStates.GetKeyAt(0) : nullptr;
        }
    }
}
