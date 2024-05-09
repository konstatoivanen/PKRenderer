#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Rendering/RHI/BuiltInResources.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "Driver.h"

// #define PK_NO_VK_VALIDATION
// #define PK_FORCE_VK_VALIDATION

namespace PK::Rendering::RHI
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::CLI;
    using namespace PK::Rendering::RHI::Objects;
    using namespace PK::Rendering::RHI::Vulkan;

    Scope<Driver> CreateRHIDriver(const std::string& workingDirectory, APIType api)
    {
        PK_THROW_ASSERT(Driver::s_instance == nullptr, "A driver instance already exists!");

        PK_LOG_HEADER("----------INITIALIZING RHI----------");
        PK_LOG_NEWLINE();
        PK_LOG_ADD_INDENT();

        CVariableRegister::Create<CVariableFunc>("RHI.Query.Memory", [](const char** args, uint32_t count)
            {
                PK_LOG_HEADER("----------GPU MEMORY INFO----------");
                auto info = Driver::Get()->GetMemoryInfo();
                PK_LOG_NEWLINE();
                PK_LOG_INFO("Block count: %i", info.blockCount);
                PK_LOG_INFO("Allocation count: %i", info.allocationCount);
                PK_LOG_INFO("Unused range count: %i", info.unusedRangeCount);
                PK_LOG_INFO("Used: %s", Functions::BytesToString(info.usedBytes).c_str());
                PK_LOG_INFO("Unused: %s", Functions::BytesToString(info.unusedBytes).c_str());
                PK_LOG_INFO("Allocation size min: %s", Functions::BytesToString(info.allocationSizeMin).c_str());
                PK_LOG_INFO("Allocation size avg: %s", Functions::BytesToString(info.allocationSizeAvg).c_str());
                PK_LOG_INFO("Allocation size max: %s", Functions::BytesToString(info.allocationSizeMax).c_str());
                PK_LOG_INFO("Unused range size min: %s", Functions::BytesToString(info.unusedRangeSizeMin).c_str());
                PK_LOG_INFO("Unused range size avg: %s", Functions::BytesToString(info.unusedRangeSizeAvg).c_str());
                PK_LOG_INFO("Unused range size max: %s", Functions::BytesToString(info.unusedRangeSizeMax).c_str());
                PK_LOG_NEWLINE();
            });

        Scope<Driver> driver = nullptr;

        switch (api)
        {
            case APIType::Vulkan:
            {
#if defined(PK_DEBUG) && !defined(PK_NO_VK_VALIDATION) || defined(PK_FORCE_VK_VALIDATION)
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
                    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
                };

                const std::vector<const char*> PK_DEVICE_EXTENTIONS =
                {
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                    VK_KHR_RAY_QUERY_EXTENSION_NAME,
                    VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
                    VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
                    VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME,
                    VK_EXT_MESH_SHADER_EXTENSION_NAME,
                    VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME
                };

                driver = CreateScope<VulkanDriver>(VulkanContextProperties
                (
                    "PK Vulkan Engine",
                    workingDirectory,
                    32ull,
                    1u,
                    3u,
                    &PK_VALIDATION_LAYERS,
                    &PK_INSTANCE_EXTENTIONS,
                    &PK_DEVICE_EXTENTIONS
                ));
            }
            break;

            default: PK_THROW_ERROR("Unsupproted graphics API"); break;
        }

        Driver::s_instance = driver.get();

        if (driver)
        {
            driver->CreateBuiltInResources();
        }

        PK_LOG_SUB_INDENT();
        PK_LOG_NEWLINE();
        PK_LOG_HEADER("----------RHI INITIALIZED----------");

        return driver;
    }

    void Driver::SetBuffer(Utilities::NameID name, Buffer* buffer) { SetBuffer(name, buffer, buffer->GetFullRange()); }
    void Driver::SetTexture(Utilities::NameID name, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(name, texture, { level, layer, 1u, 1u }); }
    void Driver::SetTexture(Utilities::NameID name, Texture* texture) { SetTexture(name, texture, {}); }
    void Driver::SetImage(Utilities::NameID name, Texture* texture, uint16_t level, uint16_t layer) { SetImage(name, texture, { level, layer, 1u, 1u }); }
    void Driver::SetImage(Utilities::NameID name, Texture* texture) { SetImage(name, texture, {}); }

    void Driver::CreateBuiltInResources() { builtInResources = new BuiltInResources(); }
    void Driver::ReleaseBuiltInResources() { delete builtInResources; }
}