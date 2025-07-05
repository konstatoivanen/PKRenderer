#pragma once
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/FixedList.h"
#include "Core/Platform/PlatformInterfaces.h"
#include "Core/Input/InputState.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/FrameStep.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, struct Sequencer)

namespace PK::App
{
    class EngineInput : public InputHandler, public IStepFrameUpdate<>
    {
    public:
        EngineInput(Sequencer* sequencer) : m_sequencer(sequencer) {}

        void OnStepFrameUpdate(FrameContext* ctx) final;

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
        FixedMap8<InputDevice*, InputState, InputStateCollection::MAX_DEVICES> m_deviceStates;
        InputState m_globalState{};
        InputDevice* m_lastDevice;
    };
}
