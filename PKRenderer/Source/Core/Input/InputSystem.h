#pragma once
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/ISingleton.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/Input/InputKey.h"
#include "Core/Input/InputDevice.h"
#include "Core/RHI/RHI.h"

namespace PK
{
    struct Sequencer;

    class InputSystem :
        public IStepApplicationUpdateInput<RHIWindow*>,
        public IStepApplicationCloseFrame<RHIWindow*>,
        public ISingleton<InputSystem>
    {
    public:
        InputSystem(Sequencer* sequencer) : m_sequencer(sequencer) {}

        InputDevice* GetDevice(RHIWindow* window);

        template<typename T>
        T* GetDevice(void* nativeHandle)
        {
            auto iter = m_inputDevices.find(nativeHandle);
            return iter != m_inputDevices.end() ? iter->second->GetNative<T>() : nullptr;
        }

        virtual void OnApplicationUpdateInput(RHIWindow* window) final;
        virtual void OnApplicationCloseFrame(RHIWindow* window) final;

    private:
        Sequencer* m_sequencer;
        std::unordered_map<void*, Unique<InputDevice>> m_inputDevices;
    };
}