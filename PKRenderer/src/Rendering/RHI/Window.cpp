#include "PrecompiledHeader.h"
#include "Window.h"
#include "Rendering/RHI/Vulkan/VulkanWindow.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "Rendering/RHI/Driver.h"

namespace PK::Rendering::RHI
{
    using namespace PK::Utilities;
    using namespace PK::Rendering;
    using namespace PK::Rendering::RHI::Vulkan;
    using namespace PK::Rendering::RHI::Vulkan::Services;

    Scope<Window> Window::Create(const WindowProperties& properties)
    {
        auto api = Driver::Get()->GetAPI();

        switch (api)
        {
            case APIType::Vulkan: return CreateScope<VulkanWindow>(RHI::Driver::GetNative<VulkanDriver>(), properties);
        }

        return nullptr;
    }

    Math::uint3 Window::GetResolutionAligned() const { return Math::Functions::GetAlignedResolution2D(GetResolution(), SIZE_ALIGNMENT); }
    Math::uint4 Window::GetRectAligned() const { return { 0u, 0u, GetResolutionAligned().xy }; };
    float Window::GetAspectRatioAligned() const { return float(GetResolutionAligned().x) / float(GetResolutionAligned().y); }
}
