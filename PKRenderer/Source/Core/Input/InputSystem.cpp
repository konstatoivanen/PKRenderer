#include "PrecompiledHeader.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/CLI/Log.h"
#include "InputSystem.h"

namespace PK
{
    void InputSystem::OnApplicationUpdateInput(RHIWindow* window)
    {
        auto state = m_deviceStates.GetValuePtr(window->GetNativeWindow());
        auto outState = InputState();

        if (state)
        {
            outState = *state;
        }

        m_sequencer->Next<InputState*>(this, &outState);
    }

    void InputSystem::OnApplicationCloseFrame(RHIWindow* window)
    {
        auto state = m_deviceStates.GetValuePtr(window->GetNativeWindow());

        if (state)
        {
            state->cursorPositionDelta = PK_FLOAT2_ZERO;
            state->keysPrevious = state->keysCurrent;
        }
    }

    void InputSystem::InputHandler_OnKey(InputDevice* device, InputKey key, bool isDown)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state && key != InputKey::None)
        {
            state->keysCurrent[(uint32_t)key] = isDown;
        }
    }

    void InputSystem::InputHandler_OnMouseMoved(InputDevice* device, const float2& position, const float2& size)
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
        }
    }

    void InputSystem::InputHandler_OnScroll(InputDevice* device, uint32_t axis, float offset)
    {
        auto state = m_deviceStates.GetValuePtr(device);

        if (state)
        {
            state->cursorScroll[axis] = offset;
        }
    }

    void InputSystem::InputHandler_OnCharacter([[maybe_unused]] InputDevice* device, [[maybe_unused]] uint32_t character)
    {
        //@TODO Do something?!?
    }

    void InputSystem::InputHandler_OnDrop([[maybe_unused]] InputDevice* device, [[maybe_unused]] const char* const* paths, [[maybe_unused]] uint32_t count)
    {
        //@TODO Do something?!?
    }

    void InputSystem::InputHandler_OnConnect(InputDevice* device)
    {
        auto index = 0u;
        if (m_deviceStates.AddKey(device, &index))
        {
            m_deviceStates.SetValueAt(index, {});
        }
    }

    void InputSystem::InputHandler_OnDisconnect(InputDevice* device)
    {
        auto statePtr = m_deviceStates.GetValuePtr(device);

        if (statePtr)
        {
            m_deviceStates.Remove(device);
        }
    }
}
