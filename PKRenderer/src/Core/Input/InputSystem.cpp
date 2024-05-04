#include "PrecompiledHeader.h"
#include "Core/Input/InputDeviceGLFW.h"
#include "Core/ControlFlow/Sequencer.h"
#include "InputSystem.h"

namespace PK::Core::Input
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Rendering::RHI;

    InputDevice* InputSystem::GetDevice(Rendering::RHI::Window* window)
    {
        // @TODO add support for other input devices
        auto nativeHandle = window->GetNativeWindow();

        auto device = GetDevice<InputDeviceGLFW>(nativeHandle);

        if (device == nullptr)
        {
            device = new InputDeviceGLFW(window);
            m_inputDevices[nativeHandle] = Utilities::Scope<InputDevice>(device);
        }

        return device;
    }

    void InputSystem::OnApplicationUpdateInput(Rendering::RHI::Window* window)
    {
        auto device = GetDevice(window);
        device->UpdateBegin();
        m_sequencer->Next<InputDevice*>(this, device);
    }

    void InputSystem::OnApplicationCloseFrame(Rendering::RHI::Window* window)
    {
        GetDevice(window)->UpdateEnd();
    }
}