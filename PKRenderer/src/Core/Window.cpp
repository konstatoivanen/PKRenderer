#include "PrecompiledHeader.h"
#include "Window.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Core
{
    using namespace PK::Rendering;
    using namespace PK::Rendering::Structs;
    using namespace PK::Rendering::VulkanRHI;
    using namespace PK::Rendering::VulkanRHI::Systems;


    Scope<Window> Window::Create(const WindowProperties& properties)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateScope<VulkanWindow>(GraphicsAPI::GetActiveDriver<VulkanDriver>(), properties);
        }

        return nullptr;
    }
}
