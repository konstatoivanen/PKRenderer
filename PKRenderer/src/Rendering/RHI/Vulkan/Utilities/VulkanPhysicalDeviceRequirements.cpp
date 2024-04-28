#include "PrecompiledHeader.h"
#include "VulkanPhysicalDeviceRequirements.h"
#include "Core/CLI/Log.h"

namespace PK::Rendering::RHI::Vulkan
{
    VulkanPhysicalDeviceFeatures::VulkanPhysicalDeviceFeatures()
    {
        vk10.pNext = &vk11;
        vk11.pNext = &vk12;
        vk12.pNext = &vk13;
        vk13.pNext = &accelerationStructure;
        accelerationStructure.pNext = &rayTracingPipeline;
        rayTracingPipeline.pNext = &rayQuery;
        rayQuery.pNext = &atomicFloat;
        atomicFloat.pNext = &positionFetch;
        positionFetch.pNext = &meshshader;
        meshshader.pNext = &shadingRate;
    }
    
    #define PK_TEST_FEATURE(field)                   \
    if (requirements.field && !available.field)      \
    {                                                \
        PK_LOG_INFO("Feature.Unavailable: " #field); \
        missingFeatures |= true;                     \
    }                                                \
    else if (requirements.field)                     \
    {                                                \
        PK_LOG_INFO("Feature.Available: " #field);   \
    }                                                \
    
    bool VulkanPhysicalDeviceFeatures::CheckRequirements(const VulkanPhysicalDeviceFeatures& requirements, const VulkanPhysicalDeviceFeatures available)
    {
        PK_LOG_INFO("VulkanPhysicalDeviceFeatures.CheckRequirements:");
        PK_LOG_ADD_INDENT();

        bool missingFeatures = false;

        PK_TEST_FEATURE(vk10.features.robustBufferAccess)
        PK_TEST_FEATURE(vk10.features.fullDrawIndexUint32)
        PK_TEST_FEATURE(vk10.features.imageCubeArray)
        PK_TEST_FEATURE(vk10.features.independentBlend)
        PK_TEST_FEATURE(vk10.features.geometryShader)
        PK_TEST_FEATURE(vk10.features.tessellationShader)
        PK_TEST_FEATURE(vk10.features.sampleRateShading)
        PK_TEST_FEATURE(vk10.features.dualSrcBlend)
        PK_TEST_FEATURE(vk10.features.logicOp)
        PK_TEST_FEATURE(vk10.features.multiDrawIndirect)
        PK_TEST_FEATURE(vk10.features.drawIndirectFirstInstance)
        PK_TEST_FEATURE(vk10.features.depthClamp)
        PK_TEST_FEATURE(vk10.features.depthBiasClamp)
        PK_TEST_FEATURE(vk10.features.fillModeNonSolid)
        PK_TEST_FEATURE(vk10.features.depthBounds)
        PK_TEST_FEATURE(vk10.features.wideLines)
        PK_TEST_FEATURE(vk10.features.largePoints)
        PK_TEST_FEATURE(vk10.features.alphaToOne)
        PK_TEST_FEATURE(vk10.features.multiViewport)
        PK_TEST_FEATURE(vk10.features.samplerAnisotropy)
        PK_TEST_FEATURE(vk10.features.textureCompressionETC2)
        PK_TEST_FEATURE(vk10.features.textureCompressionASTC_LDR)
        PK_TEST_FEATURE(vk10.features.textureCompressionBC)
        PK_TEST_FEATURE(vk10.features.occlusionQueryPrecise)
        PK_TEST_FEATURE(vk10.features.pipelineStatisticsQuery)
        PK_TEST_FEATURE(vk10.features.vertexPipelineStoresAndAtomics)
        PK_TEST_FEATURE(vk10.features.fragmentStoresAndAtomics)
        PK_TEST_FEATURE(vk10.features.shaderTessellationAndGeometryPointSize)
        PK_TEST_FEATURE(vk10.features.shaderImageGatherExtended)
        PK_TEST_FEATURE(vk10.features.shaderStorageImageExtendedFormats)
        PK_TEST_FEATURE(vk10.features.shaderStorageImageMultisample)
        PK_TEST_FEATURE(vk10.features.shaderStorageImageReadWithoutFormat)
        PK_TEST_FEATURE(vk10.features.shaderStorageImageWriteWithoutFormat)
        PK_TEST_FEATURE(vk10.features.shaderUniformBufferArrayDynamicIndexing)
        PK_TEST_FEATURE(vk10.features.shaderSampledImageArrayDynamicIndexing)
        PK_TEST_FEATURE(vk10.features.shaderStorageBufferArrayDynamicIndexing)
        PK_TEST_FEATURE(vk10.features.shaderStorageImageArrayDynamicIndexing)
        PK_TEST_FEATURE(vk10.features.shaderClipDistance)
        PK_TEST_FEATURE(vk10.features.shaderCullDistance)
        PK_TEST_FEATURE(vk10.features.shaderFloat64)
        PK_TEST_FEATURE(vk10.features.shaderInt64)
        PK_TEST_FEATURE(vk10.features.shaderInt16)
        PK_TEST_FEATURE(vk10.features.shaderResourceResidency)
        PK_TEST_FEATURE(vk10.features.shaderResourceMinLod)
        PK_TEST_FEATURE(vk10.features.sparseBinding)
        PK_TEST_FEATURE(vk10.features.sparseResidencyBuffer)
        PK_TEST_FEATURE(vk10.features.sparseResidencyImage2D)
        PK_TEST_FEATURE(vk10.features.sparseResidencyImage3D)
        PK_TEST_FEATURE(vk10.features.sparseResidency2Samples)
        PK_TEST_FEATURE(vk10.features.sparseResidency4Samples)
        PK_TEST_FEATURE(vk10.features.sparseResidency8Samples)
        PK_TEST_FEATURE(vk10.features.sparseResidency16Samples)
        PK_TEST_FEATURE(vk10.features.sparseResidencyAliased)
        PK_TEST_FEATURE(vk10.features.variableMultisampleRate)
        PK_TEST_FEATURE(vk10.features.inheritedQueries)

        PK_TEST_FEATURE(vk11.storageBuffer16BitAccess)
        PK_TEST_FEATURE(vk11.uniformAndStorageBuffer16BitAccess)
        PK_TEST_FEATURE(vk11.storagePushConstant16)
        PK_TEST_FEATURE(vk11.storageInputOutput16)
        PK_TEST_FEATURE(vk11.multiview)
        PK_TEST_FEATURE(vk11.multiviewGeometryShader)
        PK_TEST_FEATURE(vk11.multiviewTessellationShader)
        PK_TEST_FEATURE(vk11.variablePointersStorageBuffer)
        PK_TEST_FEATURE(vk11.variablePointers)
        PK_TEST_FEATURE(vk11.protectedMemory)
        PK_TEST_FEATURE(vk11.samplerYcbcrConversion)
        PK_TEST_FEATURE(vk11.shaderDrawParameters)

        PK_TEST_FEATURE(vk12.samplerMirrorClampToEdge)
        PK_TEST_FEATURE(vk12.drawIndirectCount)
        PK_TEST_FEATURE(vk12.storageBuffer8BitAccess)
        PK_TEST_FEATURE(vk12.uniformAndStorageBuffer8BitAccess)
        PK_TEST_FEATURE(vk12.storagePushConstant8)
        PK_TEST_FEATURE(vk12.shaderBufferInt64Atomics)
        PK_TEST_FEATURE(vk12.shaderSharedInt64Atomics)
        PK_TEST_FEATURE(vk12.shaderFloat16)
        PK_TEST_FEATURE(vk12.shaderInt8)
        PK_TEST_FEATURE(vk12.descriptorIndexing)
        PK_TEST_FEATURE(vk12.shaderInputAttachmentArrayDynamicIndexing)
        PK_TEST_FEATURE(vk12.shaderUniformTexelBufferArrayDynamicIndexing)
        PK_TEST_FEATURE(vk12.shaderStorageTexelBufferArrayDynamicIndexing)
        PK_TEST_FEATURE(vk12.shaderUniformBufferArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.shaderSampledImageArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.shaderStorageBufferArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.shaderStorageImageArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.shaderInputAttachmentArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.shaderUniformTexelBufferArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.shaderStorageTexelBufferArrayNonUniformIndexing)
        PK_TEST_FEATURE(vk12.descriptorBindingUniformBufferUpdateAfterBind)
        PK_TEST_FEATURE(vk12.descriptorBindingSampledImageUpdateAfterBind)
        PK_TEST_FEATURE(vk12.descriptorBindingStorageImageUpdateAfterBind)
        PK_TEST_FEATURE(vk12.descriptorBindingStorageBufferUpdateAfterBind)
        PK_TEST_FEATURE(vk12.descriptorBindingUniformTexelBufferUpdateAfterBind)
        PK_TEST_FEATURE(vk12.descriptorBindingStorageTexelBufferUpdateAfterBind)
        PK_TEST_FEATURE(vk12.descriptorBindingUpdateUnusedWhilePending)
        PK_TEST_FEATURE(vk12.descriptorBindingPartiallyBound)
        PK_TEST_FEATURE(vk12.descriptorBindingVariableDescriptorCount)
        PK_TEST_FEATURE(vk12.runtimeDescriptorArray)
        PK_TEST_FEATURE(vk12.samplerFilterMinmax)
        PK_TEST_FEATURE(vk12.scalarBlockLayout)
        PK_TEST_FEATURE(vk12.imagelessFramebuffer)
        PK_TEST_FEATURE(vk12.uniformBufferStandardLayout)
        PK_TEST_FEATURE(vk12.shaderSubgroupExtendedTypes)
        PK_TEST_FEATURE(vk12.separateDepthStencilLayouts)
        PK_TEST_FEATURE(vk12.hostQueryReset)
        PK_TEST_FEATURE(vk12.timelineSemaphore)
        PK_TEST_FEATURE(vk12.bufferDeviceAddress)
        PK_TEST_FEATURE(vk12.bufferDeviceAddressCaptureReplay)
        PK_TEST_FEATURE(vk12.bufferDeviceAddressMultiDevice)
        PK_TEST_FEATURE(vk12.vulkanMemoryModel)
        PK_TEST_FEATURE(vk12.vulkanMemoryModelDeviceScope)
        PK_TEST_FEATURE(vk12.vulkanMemoryModelAvailabilityVisibilityChains)
        PK_TEST_FEATURE(vk12.shaderOutputViewportIndex)
        PK_TEST_FEATURE(vk12.shaderOutputLayer)
        PK_TEST_FEATURE(vk12.subgroupBroadcastDynamicId)
    
        PK_TEST_FEATURE(vk13.robustImageAccess)
        PK_TEST_FEATURE(vk13.inlineUniformBlock)
        PK_TEST_FEATURE(vk13.descriptorBindingInlineUniformBlockUpdateAfterBind)
        PK_TEST_FEATURE(vk13.pipelineCreationCacheControl)
        PK_TEST_FEATURE(vk13.privateData)
        PK_TEST_FEATURE(vk13.shaderDemoteToHelperInvocation)
        PK_TEST_FEATURE(vk13.shaderTerminateInvocation)
        PK_TEST_FEATURE(vk13.subgroupSizeControl)
        PK_TEST_FEATURE(vk13.computeFullSubgroups)
        PK_TEST_FEATURE(vk13.synchronization2)
        PK_TEST_FEATURE(vk13.textureCompressionASTC_HDR)
        PK_TEST_FEATURE(vk13.shaderZeroInitializeWorkgroupMemory)
        PK_TEST_FEATURE(vk13.dynamicRendering)
        PK_TEST_FEATURE(vk13.shaderIntegerDotProduct)
        PK_TEST_FEATURE(vk13.maintenance4)

        PK_TEST_FEATURE(accelerationStructure.accelerationStructure)
        PK_TEST_FEATURE(accelerationStructure.accelerationStructureCaptureReplay)
        PK_TEST_FEATURE(accelerationStructure.accelerationStructureIndirectBuild)
        PK_TEST_FEATURE(accelerationStructure.accelerationStructureHostCommands)
        PK_TEST_FEATURE(accelerationStructure.descriptorBindingAccelerationStructureUpdateAfterBind)

        PK_TEST_FEATURE(rayTracingPipeline.rayTracingPipeline)
        PK_TEST_FEATURE(rayTracingPipeline.rayTracingPipelineShaderGroupHandleCaptureReplay)
        PK_TEST_FEATURE(rayTracingPipeline.rayTracingPipelineShaderGroupHandleCaptureReplayMixed)
        PK_TEST_FEATURE(rayTracingPipeline.rayTracingPipelineTraceRaysIndirect)
        PK_TEST_FEATURE(rayTracingPipeline.rayTraversalPrimitiveCulling)

        PK_TEST_FEATURE(rayQuery.rayQuery)

        PK_TEST_FEATURE(atomicFloat.shaderBufferFloat32Atomics)
        PK_TEST_FEATURE(atomicFloat.shaderBufferFloat32AtomicAdd)
        PK_TEST_FEATURE(atomicFloat.shaderBufferFloat64Atomics)
        PK_TEST_FEATURE(atomicFloat.shaderBufferFloat64AtomicAdd)
        PK_TEST_FEATURE(atomicFloat.shaderSharedFloat32Atomics)
        PK_TEST_FEATURE(atomicFloat.shaderSharedFloat32AtomicAdd)
        PK_TEST_FEATURE(atomicFloat.shaderSharedFloat64Atomics)
        PK_TEST_FEATURE(atomicFloat.shaderSharedFloat64AtomicAdd)
        PK_TEST_FEATURE(atomicFloat.shaderImageFloat32Atomics)
        PK_TEST_FEATURE(atomicFloat.shaderImageFloat32AtomicAdd)
        PK_TEST_FEATURE(atomicFloat.sparseImageFloat32Atomics)
        PK_TEST_FEATURE(atomicFloat.sparseImageFloat32AtomicAdd)

        PK_TEST_FEATURE(positionFetch.rayTracingPositionFetch)

        PK_TEST_FEATURE(meshshader.taskShader)
        PK_TEST_FEATURE(meshshader.meshShader)
        PK_TEST_FEATURE(meshshader.multiviewMeshShader)
        PK_TEST_FEATURE(meshshader.primitiveFragmentShadingRateMeshShader)
        PK_TEST_FEATURE(meshshader.meshShaderQueries)

        PK_TEST_FEATURE(shadingRate.pipelineFragmentShadingRate)
        PK_TEST_FEATURE(shadingRate.primitiveFragmentShadingRate)
        PK_TEST_FEATURE(shadingRate.attachmentFragmentShadingRate)

        PK_LOG_SUB_INDENT();

        if (missingFeatures)
        {
            PK_LOG_INFO("Feature requirement missmatch.");
        }
        else
        {
            PK_LOG_INFO("Feature requirements fulfilled.");
        }

        PK_LOG_NEWLINE();

        return !missingFeatures;
    }
    
    #undef PK_TEST_FEATURE
}