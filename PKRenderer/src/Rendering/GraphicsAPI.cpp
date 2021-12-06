#include "PrecompiledHeader.h"
#include "GraphicsAPI.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Utilities/Log.h"

namespace PK::Rendering
{
    using namespace Structs;
    using namespace VulkanRHI;
    using namespace PK::Utilities;

    static GraphicsDriver* s_currentDriver;

    Scope<GraphicsDriver> GraphicsDriver::Create(APIType api)
    {
        switch (api)
        {
            case APIType::Vulkan: 
                auto driver = CreateScope<VulkanDriver>(VulkanContextProperties
                (
                    "PK Vulkan Engine",
                    &PK_VALIDATION_LAYERS,
                    &PK_INSTANCE_EXTENTIONS,
                    &PK_DEVICE_EXTENTIONS,
                    VK_REQUIRED_VERSION_MAJOR,
                    VK_REQUIRED_VERSION_MINOR
                ));

                s_currentDriver = driver.get();

                PK_LOG_HEADER("----------VULKAN DRIVER INITIALIZED----------");
                return driver;
        }

        return nullptr;
    }

    GraphicsDriver* GraphicsAPI::GetActiveDriver() { return s_currentDriver; }

    APIType GraphicsAPI::GetActiveAPI() { return s_currentDriver->GetAPI(); }
   
    CommandBuffer* GraphicsAPI::GetCommandBuffer() { return s_currentDriver->GetPrimaryCommandBuffer(); }
    
    size_t GraphicsAPI::GetMemoryUsageKB() { return s_currentDriver->GetMemoryUsageKB(); }
}