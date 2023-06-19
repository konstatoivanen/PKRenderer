#include "PrecompiledHeader.h"
#include "Window.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanWindow.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

namespace PK::Core
{
    using namespace Utilities;
    using namespace Rendering;
    using namespace Rendering::Structs;
    using namespace Rendering::VulkanRHI;
    using namespace Rendering::VulkanRHI::Services;

    Scope<Window> Window::Create(const WindowProperties& properties)
    {
        auto api = GraphicsAPI::GetActiveAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateScope<VulkanWindow>(GraphicsAPI::GetActiveDriver<VulkanDriver>(), properties);
        }

        return nullptr;
    }

    Math::uint3 Window::GetResolutionAligned() const { return Math::Functions::GetAlignedResolution2D(GetResolution(), SIZE_ALIGNMENT); }
    Math::uint4 Window::GetRectAligned() const { return { 0u, 0u, GetResolutionAligned().xy }; };
    float Window::GetAspectRatioAligned() const { return float(GetResolutionAligned().x) / float(GetResolutionAligned().y); }
}
