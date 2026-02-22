#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "RHI.h"

namespace PK
{
    RHIAccelerationStructure::~RHIAccelerationStructure() = default;
    template<> RHIBindSet<RHITexture>::~RHIBindSet() = default;
    template<> RHIBindSet<RHIBuffer>::~RHIBindSet() = default;
    RHIBuffer::~RHIBuffer() = default;
    RHIQueueSet::~RHIQueueSet() = default;
    RHISwapchain::~RHISwapchain() = default;
    RHITexture::~RHITexture() = default;
    RHIShader::~RHIShader() = default;
    RHIDriver::~RHIDriver() = default;

    RHIDriver* RHI::GetDriver() { return RHIDriver::Get(); }
    RHIAPI RHI::GetActiveAPI() { return RHIDriver::Get()->GetAPI(); }
    RHIQueueSet* RHI::GetQueues() { return RHIDriver::Get()->GetQueues(); }
    RHICommandBuffer* RHI::GetCommandBuffer(QueueType queue) { return RHIDriver::Get()->GetQueues()->GetCommandBuffer(queue); }
    RHIDriverMemoryInfo RHI::GetMemoryInfo() { return RHIDriver::Get()->GetMemoryInfo(); }
    size_t RHI::GetBufferOffsetAlignment(BufferUsage usage) { return RHIDriver::Get()->GetBufferOffsetAlignment(usage); }
    const BuiltInResources* RHI::GetBuiltInResources() { return RHIDriver::Get()->GetBuiltInResources(); }
    void RHI::WaitForIdle() { RHIDriver::Get()->WaitForIdle(); }
    void RHI::GC() { RHIDriver::Get()->GC(); }

    RHIDriverScope RHI::CreateDriver(const char* workingDirectory, const RHIDriverDescriptor& descriptor)
    {
        PK_LOG_NEWLINE();
        PK_LOG_HEADER_SCOPE("----------INITIALIZING RHI----------");

        CVariableRegister::Create<CVariableFuncSimple>("RHI.Query.Memory", []()
            {
                PK_LOG_HEADER("----------GPU MEMORY INFO----------");
                auto info = RHIDriver::Get()->GetMemoryInfo();
                PK_LOG_NEWLINE();
                PK_LOG_INFO("Block count: %i", info.blockCount);
                PK_LOG_INFO("Allocation count: %i", info.allocationCount);
                PK_LOG_INFO("Unused range count: %i", info.unusedRangeCount);
                PK_LOG_INFO("Used: %s", Math::BytesToString(info.usedBytes).c_str());
                PK_LOG_INFO("Unused: %s", Math::BytesToString(info.unusedBytes).c_str());
                PK_LOG_INFO("Allocation size min: %s", Math::BytesToString(info.allocationSizeMin).c_str());
                PK_LOG_INFO("Allocation size avg: %s", Math::BytesToString(info.allocationSizeAvg).c_str());
                PK_LOG_INFO("Allocation size max: %s", Math::BytesToString(info.allocationSizeMax).c_str());
                PK_LOG_INFO("Unused range size min: %s", Math::BytesToString(info.unusedRangeSizeMin).c_str());
                PK_LOG_INFO("Unused range size avg: %s", Math::BytesToString(info.unusedRangeSizeAvg).c_str());
                PK_LOG_INFO("Unused range size max: %s", Math::BytesToString(info.unusedRangeSizeMax).c_str());
                PK_LOG_NEWLINE();
            });

        RHIDriverScope driver = nullptr;

        switch (descriptor.api)
        {
            case RHIAPI::Vulkan:
            {
                VulkanPhysicalDeviceFeatures features{};
                features.vk10.features.alphaToOne = VK_TRUE;
                features.vk10.features.fillModeNonSolid = VK_TRUE;
                features.vk10.features.shaderImageGatherExtended = VK_TRUE;
                features.vk10.features.sparseBinding = VK_TRUE;
                features.vk10.features.sparseResidencyBuffer = VK_TRUE;
                features.vk10.features.samplerAnisotropy = VK_TRUE;
                features.vk10.features.multiViewport = VK_TRUE;
                features.vk10.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
                features.vk10.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
                features.vk10.features.shaderFloat64 = VK_TRUE;
                features.vk10.features.shaderInt16 = VK_TRUE;
                features.vk10.features.shaderInt64 = VK_TRUE;
                features.vk10.features.imageCubeArray = VK_TRUE;
                features.vk10.features.fragmentStoresAndAtomics = VK_TRUE;
                features.vk10.features.multiDrawIndirect = VK_TRUE;
                features.vk10.features.shaderStorageImageReadWithoutFormat = VK_TRUE;
                features.vk10.features.shaderStorageImageWriteWithoutFormat = VK_TRUE;
                features.vk10.features.depthClamp = VK_TRUE;
                features.vk11.storageBuffer16BitAccess = VK_TRUE;
                features.vk11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
                features.vk11.storagePushConstant16 = VK_TRUE;
                features.vk11.multiview = VK_TRUE;
                features.vk12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
                features.vk12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                features.vk12.runtimeDescriptorArray = VK_TRUE;
                features.vk12.descriptorBindingVariableDescriptorCount = VK_TRUE;
                features.vk12.descriptorBindingPartiallyBound = VK_TRUE;
                features.vk12.scalarBlockLayout = VK_TRUE;
                features.vk12.shaderFloat16 = VK_TRUE;
                features.vk12.shaderInt8 = VK_TRUE;
                features.vk12.shaderOutputViewportIndex = VK_TRUE;
                features.vk12.shaderOutputLayer = VK_TRUE;
                features.vk12.bufferDeviceAddress = VK_TRUE;
                features.vk12.timelineSemaphore = VK_TRUE;
                features.vk12.storageBuffer8BitAccess = VK_TRUE;
                features.vk12.hostQueryReset = VK_TRUE;
                features.vk13.privateData = VK_FALSE;
                features.vk13.maintenance4 = VK_TRUE;
                features.vk13.dynamicRendering = VK_TRUE;
                features.vk14.dynamicRenderingLocalRead = VK_TRUE;
                features.vk14.smoothLines = VK_TRUE;
                features.accelerationStructure.accelerationStructure = VK_TRUE;
                features.rayTracingPipeline.rayTracingPipeline = VK_TRUE;
                features.rayQuery.rayQuery = VK_TRUE;
                features.atomicFloat.shaderSharedFloat32AtomicAdd = VK_TRUE;
                features.positionFetch.rayTracingPositionFetch = VK_TRUE;
                features.meshshader.taskShader = VK_TRUE;
                features.meshshader.meshShader = VK_TRUE;
                features.meshshader.multiviewMeshShader = VK_TRUE;
                features.meshshader.primitiveFragmentShadingRateMeshShader = VK_TRUE;
                features.shadingRate.primitiveFragmentShadingRate = VK_TRUE;
                features.shadingRate.pipelineFragmentShadingRate = VK_TRUE;
                features.fifoLatestReady.presentModeFifoLatestReady = VK_TRUE;
                features.swapchainMaintenance1.swapchainMaintenance1 = VK_TRUE;
                features.presentId.presentId = VK_TRUE;
                features.presentWait.presentWait = VK_TRUE;
                //features.meshshader.meshShaderQueries;

                const char* PK_INSTANCE_EXTENTIONS[] =
                {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                    PK_VK_SURFACE_EXTENSION_NAME,
                    "VK_EXT_debug_utils",
                    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
                    "VK_KHR_surface_maintenance1"
                };

                const char* PK_DEVICE_EXTENTIONS[] =
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
                    VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
                    VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME,
                    VK_KHR_MULTIVIEW_EXTENSION_NAME,
                    VK_EXT_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME,
                    VK_KHR_PRESENT_WAIT_EXTENSION_NAME,
                    VK_KHR_PRESENT_ID_EXTENSION_NAME,
                    "VK_KHR_swapchain_maintenance1"
                };

                driver = CreateUnique<VulkanDriver>(VulkanDriverDescriptor
                (
                    "PK Renderer",
                    "PK Vulkan Engine",
                    workingDirectory,
                    descriptor,
                    features,
                    { PK_INSTANCE_EXTENTIONS, sizeof(PK_INSTANCE_EXTENTIONS) / sizeof(const char*) },
                    { PK_DEVICE_EXTENTIONS, sizeof(PK_DEVICE_EXTENTIONS) / sizeof(const char*) }
                ));
            }
            break;

            default: PK_THROW_ERROR("Unsupproted graphics API"); break;
        }

        PK_LOG_HEADER("----------RHI INITIALIZED----------");
        PK_LOG_NEWLINE();

        return driver;
    }

    RHIBufferRef RHI::CreateBuffer(size_t size, BufferUsage usage, const char* name) { return RHIDriver::Get()->CreateBuffer(size, usage, name); }
    RHITextureRef RHI::CreateTexture(const TextureDescriptor& descriptor, const char* name) { return RHIDriver::Get()->CreateTexture(descriptor, name); }
    RHIAccelerationStructureRef RHI::CreateAccelerationStructure(const char* name) { return RHIDriver::Get()->CreateAccelerationStructure(name); }
    RHIShaderScope RHI::CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) { return RHIDriver::Get()->CreateShader(base, pVariant, name); }
    RHISwapchainScope RHI::CreateSwapchain(const SwapchainDescriptor& descriptor) { return RHIDriver::Get()->CreateSwapchain(descriptor); }
    RHIBuffer* RHI::AcquireStage(size_t size) { return RHIDriver::Get()->AcquireStage(size); }
    void RHI::ReleaseStage(RHIBuffer* buffer, const FenceRef& fence) { RHIDriver::Get()->ReleaseStage(buffer, fence); }

    template<> RHITextureBindSetRef RHI::CreateBindSet(size_t capacity) { return RHIDriver::Get()->CreateTextureBindSet(capacity); }
    template<> RHIBufferBindSetRef RHI::CreateBindSet(size_t capacity) { return RHIDriver::Get()->CreateBufferBindSet(capacity); }

    bool RHI::ValidateTexture(RHITextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name)
    {
        if (inoutTexture == nullptr)
        {
            inoutTexture = RHI::CreateTexture(descriptor, name);
            return true;
        }

        auto& currentDesc = inoutTexture->GetDescriptor();

        if (currentDesc.type != descriptor.type ||
            currentDesc.format != descriptor.format ||
            currentDesc.usage != descriptor.usage ||
            currentDesc.resolution != descriptor.resolution ||
            currentDesc.levels != descriptor.levels ||
            currentDesc.samples != descriptor.samples ||
            currentDesc.layers != descriptor.layers)
        {
            inoutTexture = RHI::CreateTexture(descriptor, inoutTexture->GetDebugName());
            return true;
        }

        if (currentDesc.sampler != descriptor.sampler)
        {
            inoutTexture->SetSampler(descriptor.sampler);
        }

        return false;
    }

    bool RHI::ValidateTexture(RHITextureRef& inoutTexture, const uint3& resolution)
    {
        PK_DEBUG_THROW_ASSERT(inoutTexture, "Cant partially validate a texture that hasnt been fully initialized with a descriptor!");

        if (inoutTexture->GetResolution() == resolution)
        {
            return false;
        }

        auto descriptor = inoutTexture->GetDescriptor();
        descriptor.resolution = resolution;
        inoutTexture = RHI::CreateTexture(descriptor, inoutTexture->GetDebugName());
        return true;
    }

    bool RHI::ValidateTexture(RHITextureRef& inoutTexture, const uint3& resolution, const uint32_t levels)
    {
        PK_DEBUG_THROW_ASSERT(inoutTexture, "Cant partially validate a texture that hasnt been fully initialized with a descriptor!");

        if (inoutTexture->GetResolution() == resolution && inoutTexture->GetLevels() == levels)
        {
            return false;
        }

        auto descriptor = inoutTexture->GetDescriptor();
        descriptor.resolution = resolution;
        descriptor.levels = levels;
        inoutTexture = RHI::CreateTexture(descriptor, inoutTexture->GetDebugName());
        return true;
    }

    bool RHI::ValidateTexture(RHITextureRef& inoutTexture, const uint32_t levels, const uint32_t layers)
    {
        PK_DEBUG_THROW_ASSERT(inoutTexture, "Cant partially validate a texture that hasnt been fully initialized with a descriptor!");

        if (inoutTexture->GetLevels() == levels && 
            inoutTexture->GetLayers() == layers)
        {
            return false;
        }

        auto descriptor = inoutTexture->GetDescriptor();
        descriptor.levels = levels;
        descriptor.layers = layers;
        inoutTexture = RHI::CreateTexture(descriptor, inoutTexture->GetDebugName());
        return true;
    }

    bool RHI::ValidateBuffer(RHIBufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name)
    {
        if (inoutBuffer == nullptr)
        {
            inoutBuffer = RHI::CreateBuffer(size, usage, name);
            return true;
        }

        if (inoutBuffer->GetSize() >= size && inoutBuffer->GetUsage() == usage)
        {
            return false;
        }

        inoutBuffer = RHI::CreateBuffer(size, inoutBuffer->GetUsage(), inoutBuffer->GetDebugName());
        return true;
    }

    bool RHI::ValidateBuffer(RHIBufferRef& inoutBuffer, size_t size)
    {
        PK_DEBUG_THROW_ASSERT(inoutBuffer, "Cant partially validate a buffer that hasnt been fully initialized!");

        if (inoutBuffer->GetSize() >= size)
        {
            return false;
        }

        inoutBuffer = RHI::CreateBuffer(size, inoutBuffer->GetUsage(), inoutBuffer->GetDebugName());
        return true;
    }

    void RHI::SetBuffer(NameID name, RHIBuffer* buffer, const BufferIndexRange& range) { RHIDriver::Get()->SetBuffer(name, buffer, range); }
    void RHI::SetBuffer(NameID name, RHIBuffer* buffer) { RHIDriver::Get()->SetBuffer(name, buffer, buffer->GetFullRange()); }
    void RHI::SetTexture(NameID name, RHITexture* texture, const TextureViewRange& range) { RHIDriver::Get()->SetTexture(name, texture, range); }
    void RHI::SetTexture(NameID name, RHITexture* texture, uint16_t level, uint16_t layer) { RHIDriver::Get()->SetTexture(name, texture, { level, layer, 1u, 1u }); }
    void RHI::SetTexture(NameID name, RHITexture* texture) { RHIDriver::Get()->SetTexture(name, texture, {}); }
    void RHI::SetImage(NameID name, RHITexture* texture, const TextureViewRange& range) { RHIDriver::Get()->SetImage(name, texture, range); }
    void RHI::SetImage(NameID name, RHITexture* texture, uint16_t level, uint16_t layer) { RHIDriver::Get()->SetImage(name, texture, { level, layer, 1u, 1u }); }
    void RHI::SetImage(NameID name, RHITexture* texture) { RHIDriver::Get()->SetImage(name, texture, {}); }
    void RHI::SetSampler(NameID name, const SamplerDescriptor& sampler) { RHIDriver::Get()->SetSampler(name, sampler); }
    void RHI::SetAccelerationStructure(NameID name, RHIAccelerationStructure* structure) { RHIDriver::Get()->SetAccelerationStructure(name, structure); }
    void RHI::SetBufferSet(NameID name, RHIBindSet<RHIBuffer>* bufferSet) { RHIDriver::Get()->SetBufferSet(name, bufferSet); }
    void RHI::SetTextureSet(NameID name, RHIBindSet<RHITexture>* textureSet) { RHIDriver::Get()->SetTextureSet(name, textureSet); }
    void RHI::SetConstant(NameID name, const void* data, uint32_t size) { RHIDriver::Get()->SetConstant(name, data, size); }
    void RHI::SetKeyword(NameID name, bool value) { RHIDriver::Get()->SetKeyword(name, value); }
}
