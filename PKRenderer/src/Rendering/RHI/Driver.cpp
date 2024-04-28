#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Services/StringHashID.h"
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
    using namespace PK::Core::CLI;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::RHI::Objects;
    using namespace PK::Rendering::RHI::Vulkan;

    Utilities::Scope<Driver> PK::Rendering::RHI::CreateRHIDriver(const std::string& workingDirectory, APIType api)
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

        Utilities::Scope<Driver> driver = nullptr;
    
        switch (api)
        {
            case APIType::Vulkan:
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
    
    void Driver::SetBuffer(uint32_t nameHashId, Buffer* buffer) { SetBuffer(nameHashId, buffer, buffer->GetFullRange()); }
    void Driver::SetBuffer(const char* name, Buffer* buffer, const IndexRange& range) { SetBuffer(StringHashID::StringToID(name), buffer, range); }
    void Driver::SetBuffer(const char* name, Buffer* buffer) { SetBuffer(StringHashID::StringToID(name), buffer, buffer->GetFullRange()); }
    void Driver::SetTexture(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(nameHashId, texture, { level, layer, 1u, 1u }); }
    void Driver::SetTexture(uint32_t nameHashId, Texture* texture) { SetTexture(nameHashId, texture, {}); }
    void Driver::SetTexture(const char* name, Texture* texture, const TextureViewRange& range) { SetTexture(StringHashID::StringToID(name), texture, range); }
    void Driver::SetTexture(const char* name, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(StringHashID::StringToID(name), texture, level, layer); }
    void Driver::SetTexture(const char* name, Texture* texture) { SetTexture(StringHashID::StringToID(name), texture); }
    void Driver::SetBufferArray(const char* name, BindArray<Buffer>* bufferArray){ SetBufferArray(StringHashID::StringToID(name), bufferArray); }
    void Driver::SetTextureArray(const char* name, BindArray<Texture>* textureArray) { SetTextureArray(StringHashID::StringToID(name), textureArray); }
    void Driver::SetImage(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { SetImage(nameHashId, texture, { level, layer, 1u, 1u }); }
    void Driver::SetImage(uint32_t nameHashId, Texture* texture) { SetImage(nameHashId, texture, {}); }
    void Driver::SetImage(const char* name, Texture* texture, const TextureViewRange& range) { SetImage(StringHashID::StringToID(name), texture, range); }
    void Driver::SetImage(const char* name, Texture* texture, uint16_t level, uint16_t layer) { SetImage(StringHashID::StringToID(name), texture, level, layer); }
    void Driver::SetImage(const char* name, Texture* texture) { SetImage(StringHashID::StringToID(name), texture); }
    void Driver::SetSampler(const char* name, const SamplerDescriptor& sampler) { SetSampler(StringHashID::StringToID(name), sampler); }
    void Driver::SetAccelerationStructure(const char* name, AccelerationStructure* structure) { SetAccelerationStructure(StringHashID::StringToID(name), structure); }
    void Driver::SetConstant(const char* name, const void* data, uint32_t size) { SetConstant(StringHashID::StringToID(name), data, size); }
    void Driver::SetKeyword(const char* name, bool value) { SetKeyword(StringHashID::StringToID(name), value); }

    void Driver::CreateBuiltInResources() { builtInResources = new BuiltInResources(); }
    void Driver::ReleaseBuiltInResources() { delete builtInResources; }
}