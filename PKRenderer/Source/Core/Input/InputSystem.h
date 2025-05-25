#pragma once
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Platform/PlatformInterfaces.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/Input/InputState.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct Sequencer;

    class InputSystem :
        public IStepApplicationUpdateInput<Window*>,
        public ISingleton<InputSystem>,
        public InputHandler
    {
    public:
        InputSystem(Sequencer* sequencer) : m_sequencer(sequencer) {}

        // @TODO this dependency is a bit bad. assumes usage within application step context.
        virtual void OnApplicationUpdateInput(Window* window) final;

        InputState GetInputState(InputDevice* preferredDevice, bool preferLatest, bool preferGlobal);

        void InputHandler_OnPoll() final;
        void InputHandler_OnPoll(InputDevice* device) final;
        void InputHandler_OnKey(InputDevice* device, InputKey key, bool isDown) final;
        void InputHandler_OnMouseMoved(InputDevice* device, const float2& position, const float2& areaSize) final;
        void InputHandler_OnScroll(InputDevice* device, uint32_t axis, float offset) final;
        void InputHandler_OnCharacter(InputDevice* device, uint32_t character) final;
        void InputHandler_OnDrop(InputDevice* device, const char* const* paths, uint32_t count) final;
        void InputHandler_OnConnect(InputDevice* device) final;
        void InputHandler_OnDisconnect(InputDevice* device) final;

    private:
        Sequencer* m_sequencer;
        InputDevice* m_activeInputDevice = nullptr;
        InputState m_globalInputState;
        FastMap<InputDevice*, InputState> m_deviceStates;
    };
}