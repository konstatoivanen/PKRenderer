#include "PrecompiledHeader.h"
#include "Utilities/Ref.h"
#include "Core/Services/Log.h"
#include "Core/Services/StringHashID.h"
#include "GraphicsAPI.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"


namespace PK::Rendering
{
    using namespace Core::Services;
    using namespace Rendering::Objects;
    using namespace Structs;
    using namespace VulkanRHI;
    using namespace Utilities;

    // #define PK_NO_VK_VALIDATION
    // #define PK_FORCE_VK_VALIDATION

    static GraphicsDriver* s_currentDriver;

    Scope<GraphicsDriver> GraphicsDriver::Create(const std::string& workingDirectory, APIType api)
    {
        Scope<GraphicsDriver> driver = nullptr;

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
                    VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
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

                s_currentDriver = driver.get();
                PK_LOG_HEADER("----------VULKAN DRIVER INITIALIZED----------");
        }

        if (driver)
        {
            driver->CreateBuiltInResources();
        }

        return driver;
    }

    GraphicsDriver* GraphicsAPI::GetActiveDriver() { return s_currentDriver; }
    APIType GraphicsAPI::GetActiveAPI() { return s_currentDriver->GetAPI(); }
    QueueSet* GraphicsAPI::GetQueues() { return s_currentDriver->GetQueues(); }
    DriverMemoryInfo GraphicsAPI::GetMemoryInfo() { return s_currentDriver->GetMemoryInfo(); }
    size_t GraphicsAPI::GetBufferOffsetAlignment(BufferUsage usage) { return s_currentDriver->GetBufferOffsetAlignment(usage); }
    const BuiltInResources* GraphicsAPI::GetBuiltInResources() { return s_currentDriver->builtInResources; }

    void GraphicsAPI::SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range) { s_currentDriver->SetBuffer(nameHashId, buffer, range); }
    void GraphicsAPI::SetBuffer(uint32_t nameHashId, Buffer* buffer) { s_currentDriver->SetBuffer(nameHashId, buffer, buffer->GetFullRange()); }
    void GraphicsAPI::SetBuffer(const char* name, Buffer* buffer) { SetBuffer(StringHashID::StringToID(name), buffer, buffer->GetFullRange()); }
    void GraphicsAPI::SetBuffer(const char* name, Buffer* buffer, const IndexRange& range) { SetBuffer(StringHashID::StringToID(name), buffer, range); }
    void GraphicsAPI::SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) { s_currentDriver->SetTexture(nameHashId, texture, range); }
    void GraphicsAPI::SetTexture(uint32_t nameHashId, Texture* texture) { SetTexture(nameHashId, texture, {}); }
    void GraphicsAPI::SetTexture(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(nameHashId, texture, { level, layer, 1u, 1u }); }
    void GraphicsAPI::SetTexture(const char* name, Texture* texture) { SetTexture(StringHashID::StringToID(name), texture); }
    void GraphicsAPI::SetTexture(const char* name, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(StringHashID::StringToID(name), texture, level, layer); }
    void GraphicsAPI::SetTexture(const char* name, Texture* texture, const TextureViewRange& range) { SetImage(StringHashID::StringToID(name), texture, range); }
    void GraphicsAPI::SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) { s_currentDriver->SetImage(nameHashId, texture, range); }
    void GraphicsAPI::SetImage(uint32_t nameHashId, Texture* texture) { SetImage(nameHashId, texture, {}); }
    void GraphicsAPI::SetImage(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { SetImage(nameHashId, texture, { level, layer, 1u, 1u }); }
    void GraphicsAPI::SetImage(const char* name, Texture* texture) { SetImage(StringHashID::StringToID(name), texture); }
    void GraphicsAPI::SetImage(const char* name, Texture* texture, uint16_t level, uint16_t layer) { SetImage(StringHashID::StringToID(name), texture, level, layer); }
    void GraphicsAPI::SetImage(const char* name, Texture* texture, const TextureViewRange& range) { SetImage(StringHashID::StringToID(name), texture, range); }
    void GraphicsAPI::SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure) { s_currentDriver->SetAccelerationStructure(nameHashId, structure); }
    void GraphicsAPI::SetAccelerationStructure(const char* name, AccelerationStructure* structure) { SetAccelerationStructure(StringHashID::StringToID(name), structure); }
    void GraphicsAPI::SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray) { s_currentDriver->SetBufferArray(nameHashId, bufferArray); }
    void GraphicsAPI::SetBufferArray(const char* name, BindArray<Buffer>* bufferArray) { SetBufferArray(StringHashID::StringToID(name), bufferArray); }
    void GraphicsAPI::SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray) { s_currentDriver->SetTextureArray(nameHashId, textureArray); }
    void GraphicsAPI::SetTextureArray(const char* name, BindArray<Texture>* textureArray) { SetTextureArray(StringHashID::StringToID(name), textureArray); }
    void GraphicsAPI::SetConstant(uint32_t nameHashId, const void* data, uint32_t size) { s_currentDriver->SetConstant(nameHashId, data, size); }
    void GraphicsAPI::SetConstant(const char* name, const void* data, uint32_t size) { SetConstant(StringHashID::StringToID(name), data, size); }
    void GraphicsAPI::SetKeyword(uint32_t nameHashId, bool value) { s_currentDriver->SetKeyword(nameHashId, value); }
    void GraphicsAPI::SetKeyword(const char* name, bool value) { SetKeyword(StringHashID::StringToID(name), value); }

    void GraphicsAPI::GC() { s_currentDriver->GC(); }
}