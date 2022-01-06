#include "PrecompiledHeader.h"
#include "Utilities/Ref.h"
#include "Core/Services/Log.h"
#include "GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"

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
                #ifdef PK_DEBUG
                    const std::vector<const char*> PK_VALIDATION_LAYERS =
                    {
                        "VK_LAYER_KHRONOS_validation"
                    };
                #else
                    const std::vector<const char*> PK_VALIDATION_LAYERS = {};
                #endif

                const std::vector<const char*> PK_INSTANCE_EXTENTIONS =
                {
                    "VK_EXT_debug_utils",
                };

                const std::vector<const char*> PK_DEVICE_EXTENTIONS =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                };

                auto driver = CreateScope<VulkanDriver>(VulkanContextProperties
                (
                    "PK Vulkan Engine",
                    32ull,
                    &PK_VALIDATION_LAYERS,
                    &PK_INSTANCE_EXTENTIONS,
                    &PK_DEVICE_EXTENTIONS
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
    
    DriverMemoryInfo GraphicsAPI::GetMemoryInfo() { return s_currentDriver->GetMemoryInfo(); }

    size_t GraphicsAPI::GetBufferOffsetAlignment(BufferUsage usage) { return s_currentDriver->GetBufferOffsetAlignment(usage); }
    
    void GraphicsAPI::GC() { s_currentDriver->GC(); }
}