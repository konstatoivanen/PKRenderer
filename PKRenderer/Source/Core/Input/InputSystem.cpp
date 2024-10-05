#include "PrecompiledHeader.h"
#include "Core/Input/InputDeviceGLFW.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/RHI/RHInterfaces.h"
#include "InputSystem.h"

namespace PK
{
    InputDevice* InputSystem::GetDevice(RHIWindow* window)
    {
        // @TODO add support for other input devices
        auto nativeHandle = window->GetNativeWindow();

        auto device = GetDevice<InputDeviceGLFW>(nativeHandle);

        if (device == nullptr)
        {
            device = new InputDeviceGLFW(window);
            m_inputDevices[nativeHandle] = Unique<InputDevice>(device);
        }

        return device;
    }

    void InputSystem::OnApplicationUpdateInput(RHIWindow* window)
    {
        auto device = GetDevice(window);
        device->UpdateBegin();
        m_sequencer->Next<InputDevice*>(this, device);
    }

    void InputSystem::OnApplicationCloseFrame(RHIWindow* window)
    {
        GetDevice(window)->UpdateEnd();
    }
}