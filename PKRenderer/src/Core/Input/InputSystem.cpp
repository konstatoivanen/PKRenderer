#include "PrecompiledHeader.h"
#include "Core/Input/InputDeviceGLFW.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Graphics/RHI/RHIWindow.h"
#include "InputSystem.h"

namespace PK::Core::Input
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Graphics::RHI;

    InputDevice* InputSystem::GetDevice(Graphics::Window* window)
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

    void InputSystem::OnApplicationUpdateInput(Graphics::Window* window)
    {
        auto device = GetDevice(window);
        device->UpdateBegin();
        m_sequencer->Next<InputDevice*>(this, device);
    }

    void InputSystem::OnApplicationCloseFrame(Graphics::Window* window)
    {
        GetDevice(window)->UpdateEnd();
    }
}