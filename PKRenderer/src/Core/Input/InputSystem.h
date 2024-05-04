#pragma once
#include "Math/Types.h"
#include "Utilities/ForwardDeclare.h"
#include "Utilities/Ref.h"
#include "Utilities/ISingleton.h"
#include "Core/ControlFLow/IStepApplicationWindow.h"
#include "Core/Input/InputKey.h"
#include "Core/Input/InputDevice.h"
#include "Core/IService.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::ControlFlow, class Sequencer)

namespace PK::Core::Input
{
    class InputSystem : public IService,
        public ControlFlow::IStepApplicationUpdateInputWindow,
        public ControlFlow::IStepApplicationCloseFrameWindow,
        public Utilities::ISingleton<InputSystem>
    {
    public:
        InputSystem(ControlFlow::Sequencer* sequencer) : m_sequencer(sequencer) {}

        InputDevice* GetDevice(Rendering::RHI::Window* window);

        template<typename T>
        T* GetDevice(void* nativeHandle)
        {
            auto iter = m_inputDevices.find(nativeHandle);
            return iter != m_inputDevices.end() ? iter->second->GetNative<T>() : nullptr;
        }

        virtual void OnApplicationUpdateInput(Rendering::RHI::Window* window) final;
        virtual void OnApplicationCloseFrame(Rendering::RHI::Window* window) final;

    private:
        ControlFlow::Sequencer* m_sequencer;
        std::unordered_map<void*, Utilities::Scope<InputDevice>> m_inputDevices;
    };
}