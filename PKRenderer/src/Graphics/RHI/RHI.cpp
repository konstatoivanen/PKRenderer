#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Graphics/RHI/RHIAccelerationStructure.h"
#include "Graphics/RHI/RHIBindArray.h"
#include "Graphics/RHI/RHIBuffer.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/RHI/RHIDriver.h"
#include "Graphics/RHI/RHIQueueSet.h"
#include "Graphics/RHI/RHIShader.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHIWindow.h"
#include "Graphics/RHI/Vulkan/VulkanDriver.h"
#include "RHI.h"

// #define PK_NO_VK_VALIDATION
// #define PK_FORCE_VK_VALIDATION

namespace PK::Graphics::RHI
{
    using namespace PK::Assets::Shader;
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::CLI;
    using namespace PK::Graphics::RHI::Vulkan;
    
    RHIAccelerationStructure::~RHIAccelerationStructure() = default;
    template<> RHIBindArray<RHITexture>::~RHIBindArray() = default;
    template<> RHIBindArray<RHIBuffer>::~RHIBindArray() = default;
    RHIBuffer::~RHIBuffer() = default;
    RHIQueueSet::~RHIQueueSet() = default;
    RHIWindow::~RHIWindow() = default;
    RHITexture::~RHITexture() = default;
    RHIShader::~RHIShader() = default;
    RHIDriver::~RHIDriver() = default;


    RHIDriver* RHIGetDriver() { return RHIDriver::Get(); }
    APIType RHIGetActiveAPI() { return RHIDriver::Get()->GetAPI(); }
    RHIQueueSet* RHIGetQueues() { return RHIDriver::Get()->GetQueues(); }
    RHICommandBuffer* RHIGetCommandBuffer(QueueType queue) { return RHIDriver::Get()->GetQueues()->GetCommandBuffer(queue); }
    DriverMemoryInfo RHIGetMemoryInfo() { return RHIDriver::Get()->GetMemoryInfo(); }
    size_t RHIGetBufferOffsetAlignment(BufferUsage usage) { return RHIDriver::Get()->GetBufferOffsetAlignment(usage); }
    const BuiltInResources* RHIGetBuiltInResources() { return RHIDriver::Get()->GetBuiltInResources(); }
    void RHIGC() { RHIDriver::Get()->GC(); }

    RHIDriverScope RHICreateDriver(const char* workingDirectory, APIType api)
    {
        PK_LOG_HEADER("----------INITIALIZING RHI----------");
        PK_LOG_NEWLINE();
        PK_LOG_ADD_INDENT();

        CVariableRegister::Create<CVariableFunc>("RHI.Query.Memory", [](const char** args, uint32_t count)
            {
                PK_LOG_HEADER("----------GPU MEMORY INFO----------");
                auto info = RHIDriver::Get()->GetMemoryInfo();
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

        RHIDriverScope driver = nullptr;

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

        PK_LOG_SUB_INDENT();
        PK_LOG_NEWLINE();
        PK_LOG_HEADER("----------RHI INITIALIZED----------");

        return driver;
    }

    RHIBufferRef RHICreateBuffer(size_t size, BufferUsage usage, const char* name) { return RHIDriver::Get()->CreateBuffer(size, usage, name); }
    RHITextureRef RHICreateTexture(const TextureDescriptor& descriptor, const char* name) { return RHIDriver::Get()->CreateTexture(descriptor, name); }
    RHIAccelerationStructureRef RHICreateAccelerationStructure(const char* name) { return RHIDriver::Get()->CreateAccelerationStructure(name); }
    RHIShaderScope RHICreateShader(void* base, PKShaderVariant* pVariant, const char* name) { return RHIDriver::Get()->CreateShader(base, pVariant, name); }
    RHIWindowScope RHICreateWindow(const WindowDescriptor& descriptor) { return RHIDriver::Get()->CreateWindowScope(descriptor); }
    template<> RHITextureBindArrayRef RHICreateBindArray(size_t capacity) { return RHIDriver::Get()->CreateTextureBindArray(capacity); }
    template<> RHIBufferBindArrayRef RHICreateBindArray(size_t capacity) { return RHIDriver::Get()->CreateBufferBindArray(capacity); }

    bool RHIValidateTexture(RHITextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name)
    {
        if (!inoutTexture)
        {
            inoutTexture = RHICreateTexture(descriptor, name);
            return true;
        }
        else
        {
            return inoutTexture->Validate(descriptor);
        }
    }

    bool RHIValidateBuffer(RHIBufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name)
    {
        if (!inoutBuffer)
        {
            inoutBuffer = RHICreateBuffer(size, usage, name);
            return true;
        }
        else
        {
            return inoutBuffer->Validate(size);
        }
    }

    void RHISetBuffer(Utilities::NameID name, RHIBuffer* buffer, const IndexRange& range) { RHIDriver::Get()->SetBuffer(name, buffer, range); }
    void RHISetBuffer(Utilities::NameID name, RHIBuffer* buffer) { RHIDriver::Get()->SetBuffer(name, buffer, buffer->GetFullRange()); }
    void RHISetTexture(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range) { RHIDriver::Get()->SetTexture(name, texture, range); }
    void RHISetTexture(Utilities::NameID name, RHITexture* texture, uint16_t level, uint16_t layer) { RHIDriver::Get()->SetTexture(name, texture, { level, layer, 1u, 1u }); }
    void RHISetTexture(Utilities::NameID name, RHITexture* texture) { RHIDriver::Get()->SetTexture(name, texture, {}); }
    void RHISetImage(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range) { RHIDriver::Get()->SetImage(name, texture, range); }
    void RHISetImage(Utilities::NameID name, RHITexture* texture, uint16_t level, uint16_t layer) { RHIDriver::Get()->SetImage(name, texture, { level, layer, 1u, 1u }); }
    void RHISetImage(Utilities::NameID name, RHITexture* texture) { RHIDriver::Get()->SetImage(name, texture, {}); }
    void RHISetSampler(Utilities::NameID name, const SamplerDescriptor& sampler) { RHIDriver::Get()->SetSampler(name, sampler); }
    void RHISetAccelerationStructure(Utilities::NameID name, RHIAccelerationStructure* structure) { RHIDriver::Get()->SetAccelerationStructure(name, structure); }
    void RHISetBufferArray(Utilities::NameID name, RHIBindArray<RHIBuffer>* bufferArray) { RHIDriver::Get()->SetBufferArray(name, bufferArray); }
    void RHISetTextureArray(Utilities::NameID name, RHIBindArray<RHITexture>* textureArray) { RHIDriver::Get()->SetTextureArray(name, textureArray); }
    void RHISetConstant(Utilities::NameID name, const void* data, uint32_t size) { RHIDriver::Get()->SetConstant(name, data, size); }
    void RHISetKeyword(Utilities::NameID name, bool value) { RHIDriver::Get()->SetKeyword(name, value); }
}
