#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "RHI.h"

namespace PK
{
    RHIAccelerationStructure::~RHIAccelerationStructure() = default;
    RHIBuffer::~RHIBuffer() = default;
    RHISwapchain::~RHISwapchain() = default;
    RHITexture::~RHITexture() = default;
    RHIShader::~RHIShader() = default;
    RHIGraph::~RHIGraph() = default;
    RHIDriver::~RHIDriver() = default;

    RHIDriver* RHI::GetDriver() { return RHIDriver::Get(); }
    RHIAPI RHI::GetActiveAPI() { return RHIDriver::Get()->GetAPI(); }
    RHIGraph* RHI::GetGraph() { return RHIDriver::Get()->GetGraph(); }
    RHIDriverMemoryInfo RHI::GetMemoryInfo() { return RHIDriver::Get()->GetMemoryInfo(); }
    size_t RHI::GetBufferAlignment() { return RHIDriver::Get()->GetBufferAlignment(); }
    const BuiltInResources* RHI::GetBuiltInResources() { return RHIDriver::Get()->GetBuiltInResources(); }
    void RHI::WaitForIdle() { RHIDriver::Get()->WaitForIdle(); }
    void RHI::GC() { RHIDriver::Get()->GC(); }

    RHIDriverScope RHI::CreateDriver(const RHIDriverDescriptor& descriptor)
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
                PK_LOG_INFO("Used: %s", String::FormatBytes<16>(info.usedBytes).c_str());
                PK_LOG_INFO("Unused: %s", String::FormatBytes<16>(info.unusedBytes).c_str());
                PK_LOG_INFO("Allocation size min: %s", String::FormatBytes<16>(info.allocationSizeMin).c_str());
                PK_LOG_INFO("Allocation size avg: %s", String::FormatBytes<16>(info.allocationSizeAvg).c_str());
                PK_LOG_INFO("Allocation size max: %s", String::FormatBytes<16>(info.allocationSizeMax).c_str());
                PK_LOG_INFO("Unused range size min: %s", String::FormatBytes<16>(info.unusedRangeSizeMin).c_str());
                PK_LOG_INFO("Unused range size avg: %s", String::FormatBytes<16>(info.unusedRangeSizeAvg).c_str());
                PK_LOG_INFO("Unused range size max: %s", String::FormatBytes<16>(info.unusedRangeSizeMax).c_str());
                PK_LOG_NEWLINE();
            });

        RHIDriverScope driver = nullptr;

        switch (descriptor.api)
        {
            case RHIAPI::Vulkan:
            {
                VulkanPhysicalDeviceFeatures features{};
                features.vk10.features.alphaToOne = VK_TRUE;
                features.vk10.features.sampleRateShading = VK_TRUE;
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
                features.vk12.storageBuffer8BitAccess = VK_TRUE;
                features.vk12.uniformAndStorageBuffer8BitAccess = VK_TRUE;
                features.vk12.storagePushConstant8 = VK_TRUE;
                features.vk12.shaderFloat16 = VK_TRUE;
                features.vk12.shaderInt8 = VK_TRUE;
                features.vk12.scalarBlockLayout = VK_TRUE;
                features.vk12.shaderSubgroupExtendedTypes = VK_TRUE;
                features.vk12.vulkanMemoryModel = VK_TRUE;
                features.vk12.vulkanMemoryModelDeviceScope = VK_TRUE;
                features.vk12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
                features.vk12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                features.vk12.runtimeDescriptorArray = VK_TRUE;
                features.vk12.descriptorBindingVariableDescriptorCount = VK_TRUE;
                features.vk12.descriptorBindingPartiallyBound = VK_TRUE;
                features.vk12.shaderOutputViewportIndex = VK_TRUE;
                features.vk12.shaderOutputLayer = VK_TRUE;
                features.vk12.bufferDeviceAddress = VK_TRUE;
                features.vk12.timelineSemaphore = VK_TRUE;
                features.vk12.hostQueryReset = VK_TRUE;
                features.vk13.privateData = VK_FALSE;
                features.vk13.maintenance4 = VK_TRUE;
                features.vk13.dynamicRendering = VK_TRUE;
                features.vk13.synchronization2 = VK_TRUE;
                features.vk13.shaderDemoteToHelperInvocation = VK_TRUE;
                features.vk14.dynamicRenderingLocalRead = VK_TRUE;
                features.vk14.smoothLines = VK_TRUE;
                features.vk14.maintenance5 = VK_TRUE;
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
                features.maximalReconvergence.shaderMaximalReconvergence = VK_TRUE;
                features.quadControl.shaderQuadControl = VK_TRUE;
                features.descriptorHeap.descriptorHeap = VK_TRUE;
                features.unifiedLayout.unifiedImageLayouts = VK_TRUE;
                features.deviceAddressCommands.deviceAddressCommands = VK_TRUE;
                features.raytracingMaintenance1.rayTracingMaintenance1 = VK_TRUE;
                features.shaderObject.shaderObject = VK_TRUE;
                features.untypedPointers.shaderUntypedPointers = VK_TRUE;
                //features.meshshader.meshShaderQueries;

                const char* PK_INSTANCE_EXTENTIONS[] =
                {
                    VK_KHR_SURFACE_EXTENSION_NAME,
                    PK_VK_SURFACE_EXTENSION_NAME,
                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
                    VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME
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
                    VK_KHR_SHADER_MAXIMAL_RECONVERGENCE_EXTENSION_NAME,
                    VK_KHR_SHADER_QUAD_CONTROL_EXTENSION_NAME,
                    VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
                    VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
                    VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME,
                    VK_KHR_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME,
                    VK_KHR_PRESENT_WAIT_EXTENSION_NAME,
                    VK_KHR_PRESENT_ID_EXTENSION_NAME,
                    VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
                    VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
                    VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME,
                    VK_KHR_DEVICE_ADDRESS_COMMANDS_EXTENSION_NAME,
                    VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME,
                    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
                    VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME
                };

                driver = CreateUnique<VulkanDriver>(VulkanDriverDescriptor
                (
                    "PK Renderer",
                    "PK Vulkan Engine",
                    descriptor,
                    features,
                    { PK_INSTANCE_EXTENTIONS, sizeof(PK_INSTANCE_EXTENTIONS) / sizeof(const char*) },
                    { PK_DEVICE_EXTENTIONS, sizeof(PK_DEVICE_EXTENTIONS) / sizeof(const char*) }
                ));
            }
            break;

            default: PK_FATAL_ERROR("Unsupproted graphics API"); break;
        }

        PK_LOG_HEADER("----------RHI INITIALIZED----------");
        PK_LOG_NEWLINE();

        return driver;
    }

    RHIBufferRef RHI::CreateBuffer(size_t size, BufferUsage usage, const char* name) { return RHIDriver::Get()->CreateBuffer(size, usage, name); }
    RHITextureRef RHI::CreateTexture(const TextureDescriptor& descriptor, const char* name) { return RHIDriver::Get()->CreateTexture(descriptor, name); }
    RHIAccelerationStructureRef RHI::CreateAccelerationStructure(const char* name) { return RHIDriver::Get()->CreateAccelerationStructure(name); }
    RHIShaderRef RHI::CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) { return RHIDriver::Get()->CreateShader(base, pVariant, name); }
    RHISwapchainScope RHI::CreateSwapchain(const SwapchainDescriptor& descriptor) { return RHIDriver::Get()->CreateSwapchain(descriptor); }

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
            math::any(currentDesc.resolution != descriptor.resolution) ||
            currentDesc.levels != descriptor.levels ||
            currentDesc.samples != descriptor.samples ||
            currentDesc.layers != descriptor.layers)
        {
            inoutTexture = RHI::CreateTexture(descriptor, inoutTexture->GetDebugName());
            return true;
        }

        return false;
    }

    bool RHI::ValidateTexture(RHITextureRef& inoutTexture, const uint3& resolution)
    {
        PK_DEBUG_FATAL_ASSERT(inoutTexture, "Cant partially validate a texture that hasnt been fully initialized with a descriptor!");

        if (math::all(inoutTexture->GetResolution() == resolution))
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
        PK_DEBUG_FATAL_ASSERT(inoutTexture, "Cant partially validate a texture that hasnt been fully initialized with a descriptor!");

        if (math::all(inoutTexture->GetResolution() == resolution) && inoutTexture->GetLevels() == levels)
        {
            return false;
        }

        auto descriptor = inoutTexture->GetDescriptor();
        descriptor.resolution = resolution;
        descriptor.levels = (uint8_t)levels;
        inoutTexture = RHI::CreateTexture(descriptor, inoutTexture->GetDebugName());
        return true;
    }

    bool RHI::ValidateTexture(RHITextureRef& inoutTexture, const uint32_t levels, const uint32_t layers)
    {
        PK_DEBUG_FATAL_ASSERT(inoutTexture, "Cant partially validate a texture that hasnt been fully initialized with a descriptor!");

        if (inoutTexture->GetLevels() == levels && 
            inoutTexture->GetLayers() == layers)
        {
            return false;
        }

        auto descriptor = inoutTexture->GetDescriptor();
        descriptor.levels = (uint8_t)levels;
        descriptor.layers = (uint16_t)layers;
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
        PK_DEBUG_FATAL_ASSERT(inoutBuffer, "Cant partially validate a buffer that hasnt been fully initialized!");

        if (inoutBuffer->GetSize() >= size)
        {
            return false;
        }

        inoutBuffer = RHI::CreateBuffer(size, inoutBuffer->GetUsage(), inoutBuffer->GetDebugName());
        return true;
    }

    uint8_t RHI::GetSamplerIndex(FilterMode filter, WrapMode wrapMode, BorderColor borderColor, float anisotropy)
    {
        SamplerDescriptor sampler{};
        sampler.borderColor = borderColor;
        sampler.anisotropy = anisotropy;
        sampler.filterMin = filter;
        sampler.filterMag = filter;
        sampler.wrap[0] = wrapMode;
        sampler.wrap[1] = wrapMode;
        sampler.wrap[2] = wrapMode;
        return (uint8_t)GetGraph()->MakeResident(sampler);
    }

    void RHI::SetBufferArray(NameID name, RHIBuffer** buffers, const BufferIndexRange* ranges, size_t count) { GetGraph()->SetBuffers(name, buffers, ranges, count); }
    void RHI::SetBuffer(NameID name, RHIBuffer* buffer, const BufferIndexRange& range) { GetGraph()->SetBuffers(name, &buffer, &range, 1u); }
    void RHI::SetTextureArray(NameID name, RHITexture** textures, const TextureViewRange* ranges, size_t count) { GetGraph()->SetTextures(name, textures, ranges, count); }
    void RHI::SetTexture(NameID name, RHITexture* texture, const TextureViewRange& range) { GetGraph()->SetTextures(name, &texture, &range, 1u); }
    void RHI::SetImageArray(NameID name, RHITexture** images, const TextureViewRange* ranges, size_t count) { GetGraph()->SetImages(name, images, ranges, count); }
    void RHI::SetImage(NameID name, RHITexture* image, const TextureViewRange& range) { GetGraph()->SetImages(name, &image, &range, 1u); }
    void RHI::SetAccelerationStructureArray(NameID name, RHIAccelerationStructure** structures, size_t count) { GetGraph()->SetAccelerationStructures(name, structures, count); }
    void RHI::SetAccelerationStructure(NameID name, RHIAccelerationStructure* structure) { GetGraph()->SetAccelerationStructures(name, &structure, 1u); }

    void RHI::SetBuffer(NameID name, RHIBuffer* buffer) { SetBuffer(name, buffer, {}); }
    void RHI::SetTexture(NameID name, RHITexture* texture, uint16_t level, uint16_t layer) { SetTexture(name, texture, { level, layer, 1u, 1u }); }
    void RHI::SetTexture(NameID name, RHITexture* texture) { SetTexture(name, texture, {}); }
    void RHI::SetImage(NameID name, RHITexture* image, uint16_t level, uint16_t layer) { SetImage(name, image, { level, layer, 1u, 1u }); }
    void RHI::SetImage(NameID name, RHITexture* image) { SetImage(name, image, {}); }
    void RHI::SetConstant(NameID name, const void* data, uint32_t size) { GetGraph()->SetConstant(name, data, size); }
    void RHI::SetKeyword(NameID name, bool value) { GetGraph()->SetKeyword(name, value); }

    void RHI::SetBufferArray(NameID name, RHIBuffer** buffers, size_t count) 
    {
        auto ranges = PK_STACK_ALLOC(BufferIndexRange, count);
        memset(ranges, 0, sizeof(BufferIndexRange) * count);
        SetBufferArray(name, buffers, ranges, count); 
    }

    void RHI::SetTextureArray(NameID name, RHITexture** textures, size_t count) 
    {
        auto ranges = PK_STACK_ALLOC(TextureViewRange, count);
        memset(ranges, 0, sizeof(TextureViewRange) * count);
        SetTextureArray(name, textures, ranges, count); 
    }

    void RHI::SetImageArray(NameID name, RHITexture** images, size_t count) 
    { 
        auto ranges = PK_STACK_ALLOC(TextureViewRange, count);
        memset(ranges, 0, sizeof(TextureViewRange) * count);
        SetImageArray(name, images, ranges, count); 
    }

    ConstBufferView<RHITimerScope> RHI::GetTimers() { return GetGraph()->GetTimers(); }
    PropertyBlock* RHI::GetResourceState() { return GetGraph()->GetResourceState(); }
    FenceRef RHI::GetFenceRef() { return GetGraph()->GetFenceRef(); }
    void RHI::ExecuteGraph() { GetGraph()->Execute(); }
}
