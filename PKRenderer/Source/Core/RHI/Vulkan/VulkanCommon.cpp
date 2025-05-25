#include "PrecompiledHeader.h"
#include <cstdio>
#include "Core/CLI/Log.h"
#include "Core/Utilities/FixedString.h"
#define VMA_IMPLEMENTATION
#include "VulkanCommon.h"
#include <vulkan/vk_enum_string_helper.h>

PFN_vkSetDebugUtilsObjectNameEXT pkfn_vkSetDebugUtilsObjectNameEXT = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT pkfn_vkSetDebugUtilsObjectTagEXT = nullptr;
PFN_vkQueueBeginDebugUtilsLabelEXT pkfn_vkQueueBeginDebugUtilsLabelEXT = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT pkfn_vkQueueEndDebugUtilsLabelEXT = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT pkfn_vkQueueInsertDebugUtilsLabelEXT = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT pkfn_vkCmdBeginDebugUtilsLabelEXT = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT pkfn_vkCmdEndDebugUtilsLabelEXT = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT pkfn_vkCmdInsertDebugUtilsLabelEXT = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT pkfn_vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT pkfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkSubmitDebugUtilsMessageEXT pkfn_vkSubmitDebugUtilsMessageEXT = nullptr;

PFN_vkCreateAccelerationStructureKHR pkfn_vkCreateAccelerationStructureKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR pkfn_vkDestroyAccelerationStructureKHR = nullptr;
PFN_vkCmdSetRayTracingPipelineStackSizeKHR pkfn_vkCmdSetRayTracingPipelineStackSizeKHR = nullptr;
PFN_vkCmdTraceRaysIndirectKHR pkfn_vkCmdTraceRaysIndirectKHR = nullptr;
PFN_vkCmdTraceRaysKHR pkfn_vkCmdTraceRaysKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR pkfn_vkCreateRayTracingPipelinesKHR = nullptr;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pkfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR pkfn_vkGetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupStackSizeKHR pkfn_vkGetRayTracingShaderGroupStackSizeKHR = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR pkfn_vkGetAccelerationStructureDeviceAddressKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR pkfn_vkGetAccelerationStructureBuildSizesKHR = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR pkfn_vkCmdBuildAccelerationStructuresKHR = nullptr;
PFN_vkCmdCopyAccelerationStructureKHR pkfn_vkCmdCopyAccelerationStructureKHR = nullptr;
PFN_vkCmdPipelineBarrier2KHR pkfn_vkCmdPipelineBarrier2KHR = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;

PFN_vkCmdDrawMeshTasksEXT pkfn_vkCmdDrawMeshTasksEXT = nullptr;
PFN_vkCmdDrawMeshTasksIndirectEXT pkfn_vkCmdDrawMeshTasksIndirectEXT = nullptr;
PFN_vkCmdDrawMeshTasksIndirectCountEXT pkfn_vkCmdDrawMeshTasksIndirectCountEXT = nullptr;

PFN_vkAcquireFullScreenExclusiveModeEXT pkfn_vkAcquireFullScreenExclusiveModeEXT = nullptr;
PFN_vkReleaseFullScreenExclusiveModeEXT pkfn_vkReleaseFullScreenExclusiveModeEXT = nullptr;

namespace PK
{
    VulkanPhysicalDeviceFeatures::VulkanPhysicalDeviceFeatures()
    {
        vk10.pNext = &vk11;
        vk11.pNext = &vk12;
        vk12.pNext = &vk13;
        vk13.pNext = &vk14;
        vk14.pNext = &accelerationStructure;
        accelerationStructure.pNext = &rayTracingPipeline;
        rayTracingPipeline.pNext = &rayQuery;
        rayQuery.pNext = &atomicFloat;
        atomicFloat.pNext = &positionFetch;
        positionFetch.pNext = &meshshader;
        meshshader.pNext = &shadingRate;
        shadingRate.pNext = &fifoLatestReady;
    }

    bool VulkanPhysicalDeviceFeatures::CheckRequirements(const VulkanPhysicalDeviceFeatures& requirements, const VulkanPhysicalDeviceFeatures available)
    {
        PK_LOG_INFO_FUNC("");

        bool missingFeatures = false;

        #define PK_TEST_FEATURE(field) \
            if (requirements.field && !available.field) \
            { \
                PK_LOG_INFO("Feature.Unavailable: " #field); \
                missingFeatures |= true; \
            } \
            else if (requirements.field) \
            { \
                PK_LOG_INFO("Feature.Available: " #field); \
            } \

        {
            PK_LOG_INDENT(PK_LOG_LVL_INFO);
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

            PK_TEST_FEATURE(vk14.globalPriorityQuery)
            PK_TEST_FEATURE(vk14.shaderSubgroupRotate)
            PK_TEST_FEATURE(vk14.shaderSubgroupRotateClustered)
            PK_TEST_FEATURE(vk14.shaderFloatControls2)
            PK_TEST_FEATURE(vk14.shaderExpectAssume)
            PK_TEST_FEATURE(vk14.rectangularLines)
            PK_TEST_FEATURE(vk14.bresenhamLines)
            PK_TEST_FEATURE(vk14.smoothLines)
            PK_TEST_FEATURE(vk14.stippledRectangularLines)
            PK_TEST_FEATURE(vk14.stippledBresenhamLines)
            PK_TEST_FEATURE(vk14.stippledSmoothLines)
            PK_TEST_FEATURE(vk14.vertexAttributeInstanceRateDivisor)
            PK_TEST_FEATURE(vk14.vertexAttributeInstanceRateZeroDivisor)
            PK_TEST_FEATURE(vk14.indexTypeUint8)
            PK_TEST_FEATURE(vk14.dynamicRenderingLocalRead)
            PK_TEST_FEATURE(vk14.maintenance5)
            PK_TEST_FEATURE(vk14.maintenance6)
            PK_TEST_FEATURE(vk14.pipelineProtectedAccess)
            PK_TEST_FEATURE(vk14.pipelineRobustness)
            PK_TEST_FEATURE(vk14.hostImageCopy)
            PK_TEST_FEATURE(vk14.pushDescriptor)

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

            PK_TEST_FEATURE(fifoLatestReady.presentModeFifoLatestReady)
        }
        
        #undef PK_TEST_FEATURE

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


    VulkanBufferCreateInfo::VulkanBufferCreateInfo(BufferUsage usage, size_t size, const VulkanQueueFamilies* families)
    {
        if (families)
        {
            queueFamilies = *families;
        }

        buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer.pQueueFamilyIndices = queueFamilies.indices;
        buffer.queueFamilyIndexCount = queueFamilies.count;
        buffer.sharingMode = (usage & BufferUsage::Concurrent) != 0 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        buffer.size = size;
        buffer.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        if ((usage & BufferUsage::TransferDst) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if ((usage & BufferUsage::TransferSrc) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if ((usage & BufferUsage::Vertex) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }

        if ((usage & BufferUsage::Index) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }

        if ((usage & BufferUsage::Constant) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        if ((usage & BufferUsage::Storage) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        if ((usage & BufferUsage::Indirect) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        if ((usage & BufferUsage::Sparse) != 0)
        {
            buffer.flags = VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
        }

        if ((usage & BufferUsage::PersistentStage) != 0)
        {
            allocation.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        if ((usage & BufferUsage::AccelerationStructure) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
        }

        if ((usage & BufferUsage::InstanceInput) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }

        if ((usage & BufferUsage::ShaderBindingTable) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
        }

        auto type = usage & BufferUsage::TypeBits;

        switch (type)
        {
            case BufferUsage::GPUOnly: allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY; break;
            case BufferUsage::CPUOnly: allocation.usage = VMA_MEMORY_USAGE_CPU_ONLY; break;
            case BufferUsage::CPUToGPU: allocation.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; break;
            case BufferUsage::GPUToCPU: allocation.usage = VMA_MEMORY_USAGE_GPU_TO_CPU; break;
            case BufferUsage::CPUCopy: allocation.usage = VMA_MEMORY_USAGE_CPU_COPY; break;
            default: break;
        }
    }

    VulkanImageCreateInfo::VulkanImageCreateInfo(const TextureDescriptor& descriptor, const VulkanQueueFamilies* families)
    {
        if (families)
        {
            queueFamilies = *families;
        }

        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = descriptor.type == TextureType::Texture3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        image.format = VulkanEnumConvert::GetFormat(descriptor.format);
        image.extent = { descriptor.resolution.x, descriptor.resolution.y, descriptor.resolution.z };
        image.mipLevels = descriptor.levels;
        image.arrayLayers = descriptor.layers;
        image.samples = VulkanEnumConvert::GetSampleCountFlags(descriptor.samples);
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = 0;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.sharingMode = (descriptor.usage & TextureUsage::Concurrent) != 0 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        image.pQueueFamilyIndices = queueFamilies.indices;
        image.queueFamilyIndexCount = queueFamilies.count;
        allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        formatAlias = VulkanEnumConvert::GetFormat(descriptor.formatAlias);

        if (descriptor.type == TextureType::Cubemap ||
            descriptor.type == TextureType::CubemapArray)
        {
            image.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (descriptor.formatAlias != TextureFormat::Invalid && descriptor.formatAlias != descriptor.format)
        {
            image.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_ALIAS_BIT;
            allocation.flags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
            // Set VK_IMAGE_CREATE_ALIAS_BIT  for block compressed formats
        }

        if ((descriptor.usage & TextureUsage::Sample) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if ((descriptor.usage & TextureUsage::Storage) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if ((descriptor.usage & TextureUsage::RTColor) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            if ((descriptor.usage & TextureUsage::Sample) != 0)
            {
                image.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }
        }

        if ((descriptor.usage & TextureUsage::RTStencil) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if ((descriptor.usage & TextureUsage::Upload) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if ((descriptor.usage & TextureUsage::RTDepth) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
    }


    VulkanImageView::VulkanImageView(VkDevice device, const VulkanImageViewCreateInfo& createInfo, const char* name) : device(device)
    {
        VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        info.pNext = nullptr;
        info.flags = 0;
        info.image = createInfo.isAlias ? createInfo.imageAlias : createInfo.image;
        info.viewType = createInfo.viewType;
        info.format = createInfo.isAlias ? createInfo.formatAlias : createInfo.format;
        info.components = createInfo.components;
        info.subresourceRange = createInfo.subresourceRange;

        VK_ASSERT_RESULT_CTX(vkCreateImageView(device, &info, nullptr, &view), "Failed to create an image view!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)view, name);

        bindHandle.image.view = view;
        bindHandle.image.image = createInfo.image;
        bindHandle.image.alias = createInfo.imageAlias;
        bindHandle.image.layout = createInfo.layout;
        bindHandle.image.format = info.format;
        bindHandle.image.extent = createInfo.extent;
        bindHandle.image.range = createInfo.subresourceRange;
        bindHandle.image.samples = createInfo.samples;
        bindHandle.image.sampler = VK_NULL_HANDLE;
        bindHandle.isConcurrent = createInfo.isConcurrent;
        bindHandle.isTracked = createInfo.isTracked;
    }

    VulkanImageView::~VulkanImageView()
    {
        vkDestroyImageView(device, view, nullptr);
    }

    VulkanRawBuffer::VulkanRawBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name) :
        persistentmap(createInfo.allocation.flags& VMA_ALLOCATION_CREATE_MAPPED_BIT),
        allocator(allocator),
        usage(createInfo.buffer.usage),
        size(createInfo.buffer.size)
    {
        if ((createInfo.buffer.flags & VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT) != 0)
        {
            // No automatic memory allocation for sparse buffers. Do note that mapping a sparse buffer will throw an error.
            VK_ASSERT_RESULT_CTX(vkCreateBuffer(device, &createInfo.buffer, nullptr, &buffer), "Failed to create a buffer!");
            VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer, name);
            memory = nullptr;
        }
        else
        {
            VK_ASSERT_RESULT_CTX(vmaCreateBuffer(allocator, &createInfo.buffer, &createInfo.allocation, &buffer, &memory, &allocationInfo), "Failed to create a buffer!");
            VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer, name);
        }

        if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
        {
            VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            addressInfo.buffer = buffer;
            deviceAddress = vkGetBufferDeviceAddress(device, &addressInfo);
        }
    }

    VulkanRawBuffer::~VulkanRawBuffer()
    {
        vmaDestroyBuffer(allocator, buffer, memory);
    }

    void VulkanRawBuffer::Invalidate(size_t offset, size_t size) const
    {
        vmaInvalidateAllocation(allocator, memory, offset, size);
    }

    void* VulkanRawBuffer::BeginMap(size_t offset) const
    {
        PK_THROW_ASSERT(memory, "Trying to map a buffer without dedicated memory!");

        void* mappedRange;

        if (persistentmap)
        {
            mappedRange = allocationInfo.pMappedData;
        }
        else
        {
            vmaMapMemory(allocator, memory, &mappedRange);
        }

        return reinterpret_cast<char*>(mappedRange) + offset;
    }

    void VulkanRawBuffer::EndMap(size_t offset, size_t size) const
    {
        PK_THROW_ASSERT(memory, "Trying to umap a buffer without dedicated memory!");

        if (!persistentmap)
        {
            vmaUnmapMemory(allocator, memory);
        }

        if (size > 0ull)
        {
            vmaFlushAllocation(allocator, memory, offset, size);
        }
    }

    VulkanRawImage::VulkanRawImage(VkDevice device, VmaAllocator allocator, const VulkanImageCreateInfo& createInfo, const char* name) :
        allocator(allocator),
        samples(createInfo.image.samples),
        format(createInfo.image.format),
        formatAlias(createInfo.formatAlias),
        type(createInfo.image.imageType),
        extent(createInfo.image.extent),
        levels(createInfo.image.mipLevels),
        layers(createInfo.image.arrayLayers)
    {
        if (createInfo.image.flags & VK_IMAGE_CREATE_ALIAS_BIT)
        {
            auto aliasInfo = createInfo.image;
            aliasInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
            VK_ASSERT_RESULT_CTX(vmaCreateImage(allocator, &aliasInfo, &createInfo.allocation, &image, &memory, nullptr), "Failed to create an image!");
            vmaCreateAliasingImage(allocator, memory, &createInfo.image, &imageAlias);
            VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)imageAlias, FixedString128({ name, ".Alias" }).c_str());
        }
        else
        {
            imageAlias = VK_NULL_HANDLE;
            VK_ASSERT_RESULT_CTX(vmaCreateImage(allocator, &createInfo.image, &createInfo.allocation, &image, &memory, nullptr), "Failed to create an image!");
        }

        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)image, name);
    }

    VulkanRawImage::~VulkanRawImage()
    {
        if (imageAlias != VK_NULL_HANDLE)
        {
            vmaDestroyImage(allocator, imageAlias, VK_NULL_HANDLE);
        }

        vmaDestroyImage(allocator, image, memory);

    }


    VulkanRawAccelerationStructure::VulkanRawAccelerationStructure(VkDevice device, const VkAccelerationStructureCreateInfoKHR& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &structure), "Failed to create acceleration structure!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)structure, name);
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, nullptr, structure };
        deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
    }

    VulkanRawAccelerationStructure::~VulkanRawAccelerationStructure()
    {
        vkDestroyAccelerationStructureKHR(device, structure, nullptr);
    }


    VulkanShaderModule::VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t sprivSize, const char* name) : device(device)
    {
        VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = sprivSize;
        createInfo.pCode = spirv;

        VK_ASSERT_RESULT_CTX(vkCreateShaderModule(device, &createInfo, nullptr, &module), "Failed to create shader module!");

        stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageInfo.stage = stage;
        stageInfo.module = module;
        stageInfo.pName = "main";

        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)module, name);
    }

    VulkanShaderModule::~VulkanShaderModule()
    {
        vkDestroyShaderModule(device, module, nullptr);
    }


    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline, name);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateComputePipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline, name);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkRayTracingPipelineCreateInfoKHR& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline, name);
    }

    VulkanPipeline::~VulkanPipeline()
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }


    VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreatePipelineLayout(device, &createInfo, nullptr, &layout), "Failed to create a pipeline layout!");
    }

    VulkanPipelineLayout::~VulkanPipelineLayout()
    {
        vkDestroyPipelineLayout(device, layout, nullptr);
    }


    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device,
        const VkDescriptorSetLayoutCreateInfo& createInfo,
        VkShaderStageFlagBits stageFlags,
        const char* name) :
        device(device),
        stageFlags(stageFlags),
        name(name)
    {
        VK_ASSERT_RESULT_CTX(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout), "Failed to create a descriptor set layout!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)layout, name);
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }


    VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool), "Failed to create a descriptor pool");
    }

    VulkanDescriptorPool::~VulkanDescriptorPool()
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }


    VulkanSampler::VulkanSampler(VkDevice device, const SamplerDescriptor& descriptor, const char* name) : device(device)
    {
        VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        info.addressModeU = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[0]);
        info.addressModeV = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[1]);
        info.addressModeW = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[2]);
        info.minLod = descriptor.mipMin;
        info.maxLod = descriptor.mipMax <= 0.0f ? VK_LOD_CLAMP_NONE : descriptor.mipMax;
        info.mipLodBias = descriptor.mipBias;
        info.maxAnisotropy = descriptor.anisotropy;
        info.anisotropyEnable = descriptor.anisotropy > 0.0f ? VK_TRUE : VK_FALSE;
        info.unnormalizedCoordinates = !descriptor.normalized;
        info.borderColor = VulkanEnumConvert::GetBorderColor(descriptor.borderColor);
        info.mipmapMode = (uint32_t)descriptor.filterMin > (uint32_t)FilterMode::Bilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.compareEnable = descriptor.comparison != Comparison::Off ? VK_TRUE : VK_FALSE;
        info.compareOp = VulkanEnumConvert::GetCompareOp(descriptor.comparison);
        info.magFilter = VulkanEnumConvert::GetFilterMode(descriptor.filterMag);
        info.minFilter = VulkanEnumConvert::GetFilterMode(descriptor.filterMin);

        if (info.unnormalizedCoordinates)
        {
            info.minLod = info.maxLod = 0.0f;
        }

        VK_ASSERT_RESULT_CTX(vkCreateSampler(device, &info, nullptr, &sampler), "Failed to create a sampler!");
        VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_SAMPLER, (uint64_t)sampler, name);
    }

    VulkanSampler::~VulkanSampler()
    {
        vkDestroySampler(device, sampler, nullptr);
    }


    VulkanQueryPool::VulkanQueryPool(VkDevice device, VkQueryType type, uint32_t size) :
        device(device),
        size(size),
        type(type),
        lastQueryFence(),
        count(0u)
    {
        VkQueryPoolCreateInfo info{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
        info.queryCount = size;
        info.queryType = type;
        VK_ASSERT_RESULT_CTX(vkCreateQueryPool(device, &info, nullptr, &pool), "Failed to create a query pool!");
        vkResetQueryPool(device, pool, 0, size);
    }

    VulkanQueryPool::~VulkanQueryPool()
    {
        vkDestroyQueryPool(device, pool, nullptr);
    }

    bool VulkanQueryPool::TryGetResults(void* outBuffer, size_t stride, VkQueryResultFlagBits flags)
    {
        if (count == 0 || !lastQueryFence.WaitInvalidate(0ull))
        {
            return false;
        }

        VK_ASSERT_RESULT_CTX(vkGetQueryPoolResults(device, pool, 0, count, count * stride, outBuffer, stride, flags), "Failed to get query results!");
        vkResetQueryPool(device, pool, 0, count);
        count = 0u;
        return true;
    }

    uint32_t VulkanQueryPool::AddQuery(const FenceRef& fence)
    {
        assert(count < size);
        lastQueryFence = fence;
        return count++;
    }


    namespace VulkanEnumConvert
    {
        VkFormat GetFormat(ElementType format)
        {
            switch (format)
            {
                case ElementType::Invalid: return VK_FORMAT_UNDEFINED;
                case ElementType::Float: return VK_FORMAT_R32_SFLOAT;
                case ElementType::Float2: return VK_FORMAT_R32G32_SFLOAT;
                case ElementType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
                case ElementType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                case ElementType::Double: return VK_FORMAT_R64_SFLOAT;
                case ElementType::Double2: return VK_FORMAT_R64G64_SFLOAT;
                case ElementType::Double3: return VK_FORMAT_R64G64B64_SFLOAT;
                case ElementType::Double4: return VK_FORMAT_R64G64B64A64_SFLOAT;
                case ElementType::Half: return VK_FORMAT_R16_SFLOAT;
                case ElementType::Half2: return VK_FORMAT_R16G16_SFLOAT;
                case ElementType::Half3: return VK_FORMAT_R16G16B16_SFLOAT;
                case ElementType::Half4: return VK_FORMAT_R16G16B16A16_SFLOAT;
                case ElementType::Int: return VK_FORMAT_R32_SINT;
                case ElementType::Int2: return VK_FORMAT_R32G32_SINT;
                case ElementType::Int3: return VK_FORMAT_R32G32B32_SINT;
                case ElementType::Int4: return VK_FORMAT_R32G32B32A32_SINT;
                case ElementType::Uint: return VK_FORMAT_R32_UINT;
                case ElementType::Uint2: return VK_FORMAT_R32G32_UINT;
                case ElementType::Uint3: return VK_FORMAT_R32G32B32_UINT;
                case ElementType::Uint4: return VK_FORMAT_R32G32B32A32_UINT;
                case ElementType::Short: return VK_FORMAT_R16_SINT;
                case ElementType::Short2: return VK_FORMAT_R16G16_SINT;
                case ElementType::Short3: return VK_FORMAT_R16G16B16_SINT;
                case ElementType::Short4: return VK_FORMAT_R16G16B16A16_SINT;
                case ElementType::Ushort: return VK_FORMAT_R16_UINT;
                case ElementType::Ushort2: return VK_FORMAT_R16G16_UINT;
                case ElementType::Ushort3: return VK_FORMAT_R16G16B16_UINT;
                case ElementType::Ushort4: return VK_FORMAT_R16G16B16A16_UINT;
                case ElementType::Long: return VK_FORMAT_R64_SINT;
                case ElementType::Long2: return VK_FORMAT_R64G64_SINT;
                case ElementType::Long3: return VK_FORMAT_R64G64B64_SINT;
                case ElementType::Long4: return VK_FORMAT_R64G64B64A64_SINT;
                case ElementType::Ulong: return VK_FORMAT_R64_UINT;
                case ElementType::Ulong2: return VK_FORMAT_R64G64_UINT;
                case ElementType::Ulong3: return VK_FORMAT_R64G64B64_UINT;
                case ElementType::Ulong4: return VK_FORMAT_R64G64B64A64_UINT;
                case ElementType::Float2x2: return VK_FORMAT_R32G32_SFLOAT;
                case ElementType::Float3x3: return VK_FORMAT_R32G32B32_SFLOAT;
                case ElementType::Float4x4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                case ElementType::Double2x2: return VK_FORMAT_R64G64_SFLOAT;
                case ElementType::Double3x3: return VK_FORMAT_R64G64B64_SFLOAT;
                case ElementType::Double4x4: return VK_FORMAT_R64G64B64A64_SFLOAT;
                case ElementType::Half2x2: return VK_FORMAT_R16G16_SFLOAT;
                case ElementType::Half3x3: return VK_FORMAT_R16G16B16_SFLOAT;
                case ElementType::Half4x4: return VK_FORMAT_R16G16B16A16_SFLOAT;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        ElementType GetUniformFormat(VkFormat format)
        {
            switch (format)
            {
                case VK_FORMAT_R32_SFLOAT: return ElementType::Float;
                case VK_FORMAT_R32G32_SFLOAT: return ElementType::Float2;
                case VK_FORMAT_R32G32B32_SFLOAT: return ElementType::Float3;
                case VK_FORMAT_R32G32B32A32_SFLOAT: return ElementType::Float4;
                case VK_FORMAT_R64_SFLOAT: return ElementType::Double;
                case VK_FORMAT_R64G64_SFLOAT: return ElementType::Double2;
                case VK_FORMAT_R64G64B64_SFLOAT: return ElementType::Double3;
                case VK_FORMAT_R64G64B64A64_SFLOAT: return ElementType::Double4;
                case VK_FORMAT_R16_SFLOAT: return ElementType::Half;
                case VK_FORMAT_R16G16_SFLOAT: return ElementType::Half2;
                case VK_FORMAT_R16G16B16_SFLOAT: return ElementType::Half3;
                case VK_FORMAT_R16G16B16A16_SFLOAT: return ElementType::Half4;
                case VK_FORMAT_R32_SINT: return ElementType::Int;
                case VK_FORMAT_R32G32_SINT: return ElementType::Int2;
                case VK_FORMAT_R32G32B32_SINT: return ElementType::Int3;
                case VK_FORMAT_R32G32B32A32_SINT: return ElementType::Int4;
                case VK_FORMAT_R32_UINT: return ElementType::Uint;
                case VK_FORMAT_R32G32_UINT: return ElementType::Uint2;
                case VK_FORMAT_R32G32B32_UINT: return ElementType::Uint3;
                case VK_FORMAT_R32G32B32A32_UINT: return ElementType::Uint4;
                case VK_FORMAT_R16_SINT: return ElementType::Short;
                case VK_FORMAT_R16G16_SINT: return ElementType::Short2;
                case VK_FORMAT_R16G16B16_SINT: return ElementType::Short3;
                case VK_FORMAT_R16G16B16A16_SINT: return ElementType::Short4;
                case VK_FORMAT_R16_UINT: return ElementType::Ushort;
                case VK_FORMAT_R16G16_UINT: return ElementType::Ushort2;
                case VK_FORMAT_R16G16B16_UINT: return ElementType::Ushort3;
                case VK_FORMAT_R16G16B16A16_UINT: return ElementType::Ushort4;
                case VK_FORMAT_R64_SINT: return ElementType::Long;
                case VK_FORMAT_R64G64_SINT: return ElementType::Long2;
                case VK_FORMAT_R64G64B64_SINT: return ElementType::Long3;
                case VK_FORMAT_R64G64B64A64_SINT: return ElementType::Long4;
                case VK_FORMAT_R64_UINT: return ElementType::Ulong;
                case VK_FORMAT_R64G64_UINT: return ElementType::Ulong2;
                case VK_FORMAT_R64G64B64_UINT: return ElementType::Ulong3;
                case VK_FORMAT_R64G64B64A64_UINT: return ElementType::Ulong4;
                default: return ElementType::Invalid;
            }
        }

        VkColorSpaceKHR GetColorSpace(ColorSpace colorSpace)
        {
            switch (colorSpace)
            {
                case ColorSpace::sRGB_NonLinear: return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                case ColorSpace::sRGB_Linear: return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
                case ColorSpace::scRGB: return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
                case ColorSpace::P3_NonLinear: return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
                case ColorSpace::P3_Linear: return VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT;
                case ColorSpace::P3_DCI_NonLinear: return VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT;
                case ColorSpace::BT709_Linear: return VK_COLOR_SPACE_BT709_LINEAR_EXT;
                case ColorSpace::BT709_NonLinear: return VK_COLOR_SPACE_BT709_NONLINEAR_EXT;
                case ColorSpace::BT2020_Linear: return VK_COLOR_SPACE_BT2020_LINEAR_EXT;
                case ColorSpace::HDR10_ST2084: return VK_COLOR_SPACE_HDR10_ST2084_EXT;
                case ColorSpace::HDR10_HLG: return VK_COLOR_SPACE_HDR10_HLG_EXT;
                case ColorSpace::DolbyVision: return VK_COLOR_SPACE_DOLBYVISION_EXT;
                case ColorSpace::AdobeRGB_Linear: return VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT;
                case ColorSpace::AdobeRGB_NonLinear: return VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
                case ColorSpace::PassThrough: return VK_COLOR_SPACE_PASS_THROUGH_EXT;
                case ColorSpace::AmdFreeSync2: return VK_COLOR_SPACE_DISPLAY_NATIVE_AMD;
                default: return VK_COLOR_SPACE_MAX_ENUM_KHR;
            }
        }

        VkPresentModeKHR GetPresentMode(VSyncMode vsyncMode)
        {
            switch (vsyncMode)
            {
                case VSyncMode::Immediate: return VK_PRESENT_MODE_IMMEDIATE_KHR;
                case VSyncMode::Mailbox: return VK_PRESENT_MODE_MAILBOX_KHR;
                case VSyncMode::Fifo: return VK_PRESENT_MODE_FIFO_KHR;
                case VSyncMode::FifoRelaxed: return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                case VSyncMode::FifoLatest: return VK_PRESENT_MODE_FIFO_LATEST_READY_EXT;
                case VSyncMode::SharedDemandRefresh: return VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR;
                case VSyncMode::SharedContinuous: return VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR;
                default: return VK_PRESENT_MODE_MAX_ENUM_KHR;
            };
        }

        ColorSpace GetColorSpace(VkColorSpaceKHR colorSpace)
        {
            switch (colorSpace)
            {
                case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return ColorSpace::sRGB_NonLinear;
                case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return ColorSpace::sRGB_Linear;
                case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT: return ColorSpace::scRGB;
                case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: return ColorSpace::P3_NonLinear;
                case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT: return ColorSpace::P3_Linear;
                case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT: return ColorSpace::P3_DCI_NonLinear;
                case VK_COLOR_SPACE_BT709_LINEAR_EXT: return ColorSpace::BT709_Linear;
                case VK_COLOR_SPACE_BT709_NONLINEAR_EXT: return ColorSpace::BT709_NonLinear;
                case VK_COLOR_SPACE_BT2020_LINEAR_EXT: return ColorSpace::BT2020_Linear;
                case VK_COLOR_SPACE_HDR10_ST2084_EXT: return ColorSpace::HDR10_ST2084;
                case VK_COLOR_SPACE_HDR10_HLG_EXT: return ColorSpace::HDR10_HLG;
                case VK_COLOR_SPACE_DOLBYVISION_EXT: return ColorSpace::DolbyVision;
                case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT: return ColorSpace::AdobeRGB_Linear;
                case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT: return ColorSpace::AdobeRGB_NonLinear;
                case VK_COLOR_SPACE_PASS_THROUGH_EXT: return ColorSpace::PassThrough;
                case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD: return ColorSpace::AmdFreeSync2;
                default: return ColorSpace::sRGB_NonLinear;
            }
        }

        VSyncMode GetVSyncMode(VkPresentModeKHR presentMode)
        {
            switch (presentMode)
            {
                case VK_PRESENT_MODE_IMMEDIATE_KHR: return VSyncMode::Immediate;
                case VK_PRESENT_MODE_MAILBOX_KHR: return VSyncMode::Mailbox;
                case VK_PRESENT_MODE_FIFO_KHR: return VSyncMode::Fifo;
                case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return VSyncMode::FifoRelaxed;
                case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT: return VSyncMode::FifoLatest;
                case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR: return VSyncMode::SharedDemandRefresh;
                case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: return VSyncMode::SharedContinuous;
                default: return VSyncMode::Immediate;
            };
        }

        VkFormat GetFormat(TextureFormat format)
        {
            switch (format)
            {
                // 8 bits per element.
                case TextureFormat::R8:                 return VK_FORMAT_R8_UNORM;
                case TextureFormat::R8_SNORM:           return VK_FORMAT_R8_SNORM;
                case TextureFormat::R8UI:               return VK_FORMAT_R8_UINT;
                case TextureFormat::R8I:                return VK_FORMAT_R8_SINT;
                case TextureFormat::Stencil8:           return VK_FORMAT_S8_UINT;
                case TextureFormat::R16F:               return VK_FORMAT_R16_SFLOAT;
                case TextureFormat::R16UI:              return VK_FORMAT_R16_UINT;
                case TextureFormat::R16I:               return VK_FORMAT_R16_SINT;
                case TextureFormat::RG8:                return VK_FORMAT_R8G8_UNORM;
                case TextureFormat::RG8_SNORM:          return VK_FORMAT_R8G8_SNORM;
                case TextureFormat::RG8UI:              return VK_FORMAT_R8G8_UINT;
                case TextureFormat::RG8I:               return VK_FORMAT_R8G8_SINT;
                case TextureFormat::RGB565:             return VK_FORMAT_R5G6B5_UNORM_PACK16;
                case TextureFormat::RGB5A1:             return VK_FORMAT_R5G5B5A1_UNORM_PACK16;
                case TextureFormat::RGBA4:              return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
                case TextureFormat::Depth16:            return VK_FORMAT_D16_UNORM;
                case TextureFormat::RGB8:               return VK_FORMAT_R8G8B8_UNORM;
                case TextureFormat::RGB8_SRGB:          return VK_FORMAT_R8G8B8_SRGB;
                case TextureFormat::RGB8_SNORM:         return VK_FORMAT_R8G8B8_SNORM;
                case TextureFormat::RGB8UI:             return VK_FORMAT_R8G8B8_UINT;
                case TextureFormat::RGB8I:              return VK_FORMAT_R8G8B8_SINT;
                case TextureFormat::R32F:               return VK_FORMAT_R32_SFLOAT;
                case TextureFormat::R32UI:              return VK_FORMAT_R32_UINT;
                case TextureFormat::R32I:               return VK_FORMAT_R32_SINT;
                case TextureFormat::RG16F:              return VK_FORMAT_R16G16_SFLOAT;
                case TextureFormat::RG16UI:             return VK_FORMAT_R16G16_UINT;
                case TextureFormat::RG16I:              return VK_FORMAT_R16G16_SINT;
                case TextureFormat::B10G11R11UF:        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                case TextureFormat::RGB9E5:             return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
                case TextureFormat::RGBA8:              return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGBA8_SRGB:         return VK_FORMAT_R8G8B8A8_SRGB;
                case TextureFormat::RGBA8_SNORM:        return VK_FORMAT_R8G8B8A8_SNORM;
                case TextureFormat::BGRA8:              return VK_FORMAT_B8G8R8A8_UNORM;
                case TextureFormat::BGRA8_SRGB:         return VK_FORMAT_B8G8R8A8_SRGB;
                case TextureFormat::RGB10A2:            return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                case TextureFormat::RGBA8UI:            return VK_FORMAT_R8G8B8A8_UINT;
                case TextureFormat::RGBA8I:             return VK_FORMAT_R8G8B8A8_SINT;
                case TextureFormat::Depth32F:           return VK_FORMAT_D32_SFLOAT;
                case TextureFormat::Depth24_Stencil8:   return VK_FORMAT_D24_UNORM_S8_UINT;
                case TextureFormat::Depth32F_Stencil8:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
                case TextureFormat::RGB16F:             return VK_FORMAT_R16G16B16_SFLOAT;
                case TextureFormat::RGB16UI:            return VK_FORMAT_R16G16B16_UINT;
                case TextureFormat::RGB16I:             return VK_FORMAT_R16G16B16_SINT;
                case TextureFormat::RG32F:              return VK_FORMAT_R32G32_SFLOAT;
                case TextureFormat::RG32UI:             return VK_FORMAT_R32G32_UINT;
                case TextureFormat::RG32I:              return VK_FORMAT_R32G32_SINT;
                case TextureFormat::RGBA16:             return VK_FORMAT_R16G16B16A16_UNORM;;
                case TextureFormat::RGBA16F:            return VK_FORMAT_R16G16B16A16_SFLOAT;
                case TextureFormat::RGBA16UI:           return VK_FORMAT_R16G16B16A16_UINT;
                case TextureFormat::RGBA16I:            return VK_FORMAT_R16G16B16A16_SINT;
                case TextureFormat::RGB32F:             return VK_FORMAT_R32G32B32_SFLOAT;
                case TextureFormat::RGB32UI:            return VK_FORMAT_R32G32B32_UINT;
                case TextureFormat::RGB32I:             return VK_FORMAT_R32G32B32_SINT;
                case TextureFormat::RGBA32F:            return VK_FORMAT_R32G32B32A32_SFLOAT;
                case TextureFormat::RGBA32UI:           return VK_FORMAT_R32G32B32A32_UINT;
                case TextureFormat::RGBA32I:            return VK_FORMAT_R32G32B32A32_SINT;
                case TextureFormat::RGBA64UI:           return VK_FORMAT_R64G64B64A64_UINT;
                case TextureFormat::BC1_RGB:            return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
                case TextureFormat::BC1_SRGB:           return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
                case TextureFormat::BC1_RGBA:           return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                case TextureFormat::BC1_SRGBA:          return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
                case TextureFormat::BC2_RGBA:           return VK_FORMAT_BC2_UNORM_BLOCK;
                case TextureFormat::BC2_SRGBA:          return VK_FORMAT_BC2_SRGB_BLOCK;
                case TextureFormat::BC3_RGBA:           return VK_FORMAT_BC3_UNORM_BLOCK;
                case TextureFormat::BC3_SRGBA:          return VK_FORMAT_BC3_SRGB_BLOCK;
                case TextureFormat::BC4:                return VK_FORMAT_BC4_UNORM_BLOCK;
                case TextureFormat::BC6H_RGBUF:         return VK_FORMAT_BC6H_UFLOAT_BLOCK;
                case TextureFormat::BC6H_RGBF:          return VK_FORMAT_BC6H_SFLOAT_BLOCK;
                case TextureFormat::BC7_UNORM:          return VK_FORMAT_BC7_UNORM_BLOCK;
                default:                                return VK_FORMAT_UNDEFINED;
            }
        }

        VkIndexType GetIndexType(ElementType format)
        {
            switch (format)
            {
                case ElementType::Ushort: return VK_INDEX_TYPE_UINT16;
                case ElementType::Uint: return VK_INDEX_TYPE_UINT32;
                default: return VK_INDEX_TYPE_MAX_ENUM;
            }
        }

        TextureFormat GetTextureFormat(VkFormat format)
        {
            switch (format)
            {
                case VK_FORMAT_R8_UNORM:                  return TextureFormat::R8;
                case VK_FORMAT_R8_SNORM:                  return TextureFormat::R8_SNORM;
                case VK_FORMAT_R8_UINT:                   return TextureFormat::R8UI;
                case VK_FORMAT_R8_SINT:                   return TextureFormat::R8I;
                case VK_FORMAT_S8_UINT:                   return TextureFormat::Stencil8;
                case VK_FORMAT_R16_SFLOAT:                return TextureFormat::R16F;
                case VK_FORMAT_R16_UINT:                  return TextureFormat::R16UI;
                case VK_FORMAT_R16_SINT:                  return TextureFormat::R16I;
                case VK_FORMAT_R8G8_UNORM:                return TextureFormat::RG8;
                case VK_FORMAT_R8G8_SNORM:                return TextureFormat::RG8_SNORM;
                case VK_FORMAT_R8G8_UINT:                 return TextureFormat::RG8UI;
                case VK_FORMAT_R8G8_SINT:                 return TextureFormat::RG8I;
                case VK_FORMAT_R5G6B5_UNORM_PACK16:       return TextureFormat::RGB565;
                case VK_FORMAT_R5G5B5A1_UNORM_PACK16:     return TextureFormat::RGB5A1;
                case VK_FORMAT_R4G4B4A4_UNORM_PACK16:     return TextureFormat::RGBA4;
                case VK_FORMAT_D16_UNORM:                 return TextureFormat::Depth16;
                case VK_FORMAT_R8G8B8_UNORM:              return TextureFormat::RGB8;
                case VK_FORMAT_R8G8B8_SRGB:               return TextureFormat::RGB8_SRGB;
                case VK_FORMAT_R8G8B8_SNORM:              return TextureFormat::RGB8_SNORM;
                case VK_FORMAT_R8G8B8_UINT:               return TextureFormat::RGB8UI;
                case VK_FORMAT_R8G8B8_SINT:               return TextureFormat::RGB8I;
                case VK_FORMAT_R32_SFLOAT:                return TextureFormat::R32F;
                case VK_FORMAT_R32_UINT:                  return TextureFormat::R32UI;
                case VK_FORMAT_R32_SINT:                  return TextureFormat::R32I;
                case VK_FORMAT_R16G16_SFLOAT:             return TextureFormat::RG16F;
                case VK_FORMAT_R16G16_UINT:               return TextureFormat::RG16UI;
                case VK_FORMAT_R16G16_SINT:               return TextureFormat::RG16I;
                case VK_FORMAT_B10G11R11_UFLOAT_PACK32:   return TextureFormat::B10G11R11UF;
                case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:    return TextureFormat::RGB9E5;
                case VK_FORMAT_R8G8B8A8_UNORM:            return TextureFormat::RGBA8;
                case VK_FORMAT_R8G8B8A8_SRGB:             return TextureFormat::RGBA8_SRGB;
                case VK_FORMAT_R8G8B8A8_SNORM:            return TextureFormat::RGBA8_SNORM;
                case VK_FORMAT_B8G8R8A8_UNORM:            return TextureFormat::BGRA8;
                case VK_FORMAT_B8G8R8A8_SRGB:             return TextureFormat::BGRA8_SRGB;
                case VK_FORMAT_A2B10G10R10_UNORM_PACK32:  return TextureFormat::RGB10A2;
                case VK_FORMAT_R8G8B8A8_UINT:             return TextureFormat::RGBA8UI;
                case VK_FORMAT_R8G8B8A8_SINT:             return TextureFormat::RGBA8I;
                case VK_FORMAT_D32_SFLOAT:                return TextureFormat::Depth32F;
                case VK_FORMAT_D24_UNORM_S8_UINT:         return TextureFormat::Depth24_Stencil8;
                case VK_FORMAT_D32_SFLOAT_S8_UINT:        return TextureFormat::Depth32F_Stencil8;
                case VK_FORMAT_R16G16B16_SFLOAT:          return TextureFormat::RGB16F;
                case VK_FORMAT_R16G16B16_UINT:            return TextureFormat::RGB16UI;
                case VK_FORMAT_R16G16B16_SINT:            return TextureFormat::RGB16I;
                case VK_FORMAT_R32G32_SFLOAT:             return TextureFormat::RG32F;
                case VK_FORMAT_R32G32_UINT:               return TextureFormat::RG32UI;
                case VK_FORMAT_R32G32_SINT:               return TextureFormat::RG32I;
                case VK_FORMAT_R16G16B16A16_UNORM:        return TextureFormat::RGBA16;
                case VK_FORMAT_R16G16B16A16_SFLOAT:       return TextureFormat::RGBA16F;
                case VK_FORMAT_R16G16B16A16_UINT:         return TextureFormat::RGBA16UI;
                case VK_FORMAT_R16G16B16A16_SINT:         return TextureFormat::RGBA16I;
                case VK_FORMAT_R32G32B32_SFLOAT:          return TextureFormat::RGB32F;
                case VK_FORMAT_R32G32B32_UINT:            return TextureFormat::RGB32UI;
                case VK_FORMAT_R32G32B32_SINT:            return TextureFormat::RGB32I;
                case VK_FORMAT_R32G32B32A32_SFLOAT:       return TextureFormat::RGBA32F;
                case VK_FORMAT_R32G32B32A32_UINT:         return TextureFormat::RGBA32UI;
                case VK_FORMAT_R32G32B32A32_SINT:         return TextureFormat::RGBA32I;
                case VK_FORMAT_R64G64B64A64_UINT:         return TextureFormat::RGBA64UI;
                case VK_FORMAT_BC1_RGB_UNORM_BLOCK:       return TextureFormat::BC1_RGB;
                case VK_FORMAT_BC1_RGB_SRGB_BLOCK:        return TextureFormat::BC1_SRGB;
                case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:      return TextureFormat::BC1_RGBA;
                case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:       return TextureFormat::BC1_SRGBA;
                case VK_FORMAT_BC2_UNORM_BLOCK:           return TextureFormat::BC2_RGBA;
                case VK_FORMAT_BC2_SRGB_BLOCK:            return TextureFormat::BC2_SRGBA;
                case VK_FORMAT_BC3_UNORM_BLOCK:           return TextureFormat::BC3_RGBA;
                case VK_FORMAT_BC3_SRGB_BLOCK:            return TextureFormat::BC3_SRGBA;
                case VK_FORMAT_BC4_UNORM_BLOCK:           return TextureFormat::BC4;
                case VK_FORMAT_BC6H_UFLOAT_BLOCK:         return TextureFormat::BC6H_RGBUF;
                case VK_FORMAT_BC6H_SFLOAT_BLOCK:         return TextureFormat::BC6H_RGBF;
                case VK_FORMAT_BC7_UNORM_BLOCK:           return TextureFormat::BC7_UNORM;
                default:                                  return TextureFormat::Invalid;
            }
        }

        uint32_t GetFormatBlockSize(VkFormat format)
        {
            switch (format)
            {
                case VK_FORMAT_R8_UNORM:
                case VK_FORMAT_R8_SNORM:
                case VK_FORMAT_R8_UINT:
                case VK_FORMAT_R8_SINT:
                case VK_FORMAT_S8_UINT:
                    return 1u;

                case VK_FORMAT_R16_SFLOAT:
                case VK_FORMAT_R16_UINT:
                case VK_FORMAT_R16_SINT:
                case VK_FORMAT_R8G8_UNORM:
                case VK_FORMAT_R8G8_SNORM:
                case VK_FORMAT_R8G8_UINT:
                case VK_FORMAT_R8G8_SINT:
                case VK_FORMAT_R5G6B5_UNORM_PACK16:
                case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
                case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
                case VK_FORMAT_D16_UNORM:
                    return 2u;

                case VK_FORMAT_R8G8B8_UNORM:
                case VK_FORMAT_R8G8B8_SRGB:
                case VK_FORMAT_R8G8B8_SNORM:
                case VK_FORMAT_R8G8B8_UINT:
                case VK_FORMAT_R8G8B8_SINT:
                    return 3u;

                case VK_FORMAT_R32_SFLOAT:
                case VK_FORMAT_R32_UINT:
                case VK_FORMAT_R32_SINT:
                case VK_FORMAT_R16G16_SFLOAT:
                case VK_FORMAT_R16G16_UINT:
                case VK_FORMAT_R16G16_SINT:
                case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                case VK_FORMAT_R8G8B8A8_UNORM:
                case VK_FORMAT_R8G8B8A8_SRGB:
                case VK_FORMAT_R8G8B8A8_SNORM:
                case VK_FORMAT_B8G8R8A8_UNORM:
                case VK_FORMAT_B8G8R8A8_SRGB:
                case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
                case VK_FORMAT_R8G8B8A8_UINT:
                case VK_FORMAT_R8G8B8A8_SINT:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                    return 4u;

                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    return 5u;

                case VK_FORMAT_R16G16B16_SFLOAT:
                case VK_FORMAT_R16G16B16_UINT:
                case VK_FORMAT_R16G16B16_SINT:
                    return 6u;

                case VK_FORMAT_R32G32_SFLOAT:
                case VK_FORMAT_R32G32_UINT:
                case VK_FORMAT_R32G32_SINT:
                case VK_FORMAT_R16G16B16A16_UNORM:
                case VK_FORMAT_R16G16B16A16_SFLOAT:
                case VK_FORMAT_R16G16B16A16_UINT:
                case VK_FORMAT_R16G16B16A16_SINT:
                    return 8u;

                case VK_FORMAT_R32G32B32_SFLOAT:
                case VK_FORMAT_R32G32B32_UINT:
                case VK_FORMAT_R32G32B32_SINT:
                    return 12u;

                case VK_FORMAT_R32G32B32A32_SFLOAT:
                case VK_FORMAT_R32G32B32A32_UINT:
                case VK_FORMAT_R32G32B32A32_SINT:
                    return 16u;

                case VK_FORMAT_R64G64B64A64_UINT:
                    return 32u;

                case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
                case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
                case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
                case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
                    return 8u;

                case VK_FORMAT_BC2_UNORM_BLOCK:
                case VK_FORMAT_BC2_SRGB_BLOCK:
                case VK_FORMAT_BC3_UNORM_BLOCK:
                case VK_FORMAT_BC3_SRGB_BLOCK: 
                    return 16u;
                
                case VK_FORMAT_BC4_UNORM_BLOCK:           
                    return 8u;
                
                case VK_FORMAT_BC6H_UFLOAT_BLOCK:
                case VK_FORMAT_BC6H_SFLOAT_BLOCK:
                case VK_FORMAT_BC7_UNORM_BLOCK:
                    return 16u;
                
                default: 
                    return 0u;
            }

            return uint32_t();
        }

        VkImageAspectFlagBits GetFormatAspect(VkFormat format)
        {
            if (IsDepthStencilFormat(format))
            {
                return (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
            }

            if (IsDepthFormat(format))
            {
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        uint32_t ExpandVkRange16(uint32_t v)
        {
            return v >= 0x7FFF ? VK_REMAINING_ARRAY_LAYERS : v;
        }


        bool IsDepthFormat(VkFormat format)
        {
            return format == VK_FORMAT_D16_UNORM ||
                format == VK_FORMAT_D16_UNORM_S8_UINT ||
                format == VK_FORMAT_D24_UNORM_S8_UINT ||
                format == VK_FORMAT_D32_SFLOAT ||
                format == VK_FORMAT_D32_SFLOAT_S8_UINT;
        }

        bool IsDepthStencilFormat(VkFormat format)
        {
            return  format == VK_FORMAT_D16_UNORM_S8_UINT ||
                format == VK_FORMAT_D24_UNORM_S8_UINT ||
                format == VK_FORMAT_D32_SFLOAT_S8_UINT;
        }

        VkClearValue GetClearValue(const TextureClearValue& clearValue)
        {
            VkClearValue outValue;
            outValue.color.uint32[0] = clearValue.uint32[0];
            outValue.color.uint32[1] = clearValue.uint32[1];
            outValue.color.uint32[2] = clearValue.uint32[2];
            outValue.color.uint32[3] = clearValue.uint32[3];
            return outValue;
        }

        VkComponentMapping GetSwizzle(VkFormat format)
        {
            switch (format)
            {
                case VK_FORMAT_R8_UNORM:
                case VK_FORMAT_R8_SNORM:
                case VK_FORMAT_R8_UINT:
                case VK_FORMAT_R8_SINT:
                case VK_FORMAT_S8_UINT:
                case VK_FORMAT_R16_SFLOAT:
                case VK_FORMAT_R16_UINT:
                case VK_FORMAT_R16_SINT:
                case VK_FORMAT_R32_SFLOAT:
                case VK_FORMAT_R32_UINT:
                case VK_FORMAT_R32_SINT:
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                case VK_FORMAT_BC4_UNORM_BLOCK:
                    return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R };

                case VK_FORMAT_R8G8_UNORM:
                case VK_FORMAT_R8G8_SNORM:
                case VK_FORMAT_R8G8_UINT:
                case VK_FORMAT_R8G8_SINT:
                case VK_FORMAT_R16G16_SFLOAT:
                case VK_FORMAT_R16G16_UINT:
                case VK_FORMAT_R16G16_SINT:
                case VK_FORMAT_R32G32_SFLOAT:
                case VK_FORMAT_R32G32_UINT:
                case VK_FORMAT_R32G32_SINT:
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO };

                case VK_FORMAT_R5G6B5_UNORM_PACK16:
                case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
                case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_R32G32B32_SFLOAT:
                case VK_FORMAT_R32G32B32_UINT:
                case VK_FORMAT_R32G32B32_SINT:
                case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
                case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
                    return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO };

                case VK_FORMAT_R8G8B8A8_UNORM:
                case VK_FORMAT_R8G8B8A8_SRGB:
                case VK_FORMAT_R8G8B8A8_SNORM:
                case VK_FORMAT_R8G8B8A8_UINT:
                case VK_FORMAT_R8G8B8A8_SINT:
                case VK_FORMAT_R16G16B16A16_UNORM:
                case VK_FORMAT_R16G16B16A16_SFLOAT:
                case VK_FORMAT_R16G16B16A16_UINT:
                case VK_FORMAT_R16G16B16A16_SINT:
                case VK_FORMAT_R32G32B32A32_SFLOAT:
                case VK_FORMAT_R32G32B32A32_UINT:
                case VK_FORMAT_R32G32B32A32_SINT:
                case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
                case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
                case VK_FORMAT_BC3_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                    return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

                case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                    return { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO };
                case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
                    return { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R };

                case VK_FORMAT_B8G8R8A8_SRGB:
                    return { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

                default: 
                    return { VK_COMPONENT_SWIZZLE_MAX_ENUM, VK_COMPONENT_SWIZZLE_MAX_ENUM, VK_COMPONENT_SWIZZLE_MAX_ENUM, VK_COMPONENT_SWIZZLE_MAX_ENUM };
            }
        }

        VkImageViewType GetViewType(TextureType type)
        {
            switch (type)
            {
                case TextureType::Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
                case TextureType::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                case TextureType::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
                case TextureType::Cubemap: return VK_IMAGE_VIEW_TYPE_CUBE;
                case TextureType::CubemapArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                default: return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
            }
        }

        VkImageLayout GetImageLayout(TextureUsage usage)
        {
            if ((usage & (TextureUsage::RTDepth | TextureUsage::RTColor | TextureUsage::Storage)) != 0)
            {
                return VK_IMAGE_LAYOUT_GENERAL;
            }

            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        VkAttachmentLoadOp GetLoadOp(LoadOp loadOp)
        {
            switch (loadOp)
            {
                case LoadOp::None: return VK_ATTACHMENT_LOAD_OP_NONE;
                case LoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
                case LoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
                case LoadOp::Discard: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                default: return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
            }
        }

        VkAttachmentStoreOp GetStoreOp(StoreOp storeOp)
        {
            switch (storeOp)
            {
                case StoreOp::None: return VK_ATTACHMENT_STORE_OP_NONE;
                case StoreOp::Store: return VK_ATTACHMENT_STORE_OP_STORE;
                case StoreOp::Discard: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
                default: return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
            }
        }

        VkCompareOp GetCompareOp(Comparison comparison)
        {
            switch (comparison)
            {
                case Comparison::Off: return VK_COMPARE_OP_ALWAYS;
                case Comparison::Never: return VK_COMPARE_OP_NEVER;
                case Comparison::Less: return VK_COMPARE_OP_LESS;
                case Comparison::Equal: return VK_COMPARE_OP_EQUAL;
                case Comparison::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
                case Comparison::Greater: return VK_COMPARE_OP_GREATER;
                case Comparison::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
                case Comparison::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case Comparison::Always: return VK_COMPARE_OP_ALWAYS;
                default: return VK_COMPARE_OP_MAX_ENUM;
            }
        }

        VkBorderColor GetBorderColor(BorderColor color)
        {
            switch (color)
            {
                case BorderColor::FloatClear: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
                case BorderColor::IntClear: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
                case BorderColor::FloatBlack: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                case BorderColor::IntBlack: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                case BorderColor::FloatWhite: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                case BorderColor::IntWhite: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
                default: return VK_BORDER_COLOR_MAX_ENUM;
            }
        }

        VkSamplerAddressMode GetSamplerAddressMode(WrapMode wrap)
        {
            switch (wrap)
            {
                case WrapMode::Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case WrapMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case WrapMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                case WrapMode::MirrorOnce: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
                case WrapMode::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
                default: return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
            }
        }

        VkFilter GetFilterMode(FilterMode filter)
        {
            switch (filter)
            {
                case FilterMode::Point: return VK_FILTER_NEAREST;
                case FilterMode::Bilinear: return VK_FILTER_LINEAR;
                case FilterMode::Trilinear: return VK_FILTER_LINEAR;
                case FilterMode::Bicubic: return VK_FILTER_CUBIC_IMG;
                default: return VK_FILTER_MAX_ENUM;
            }
        }

        VkDescriptorType GetDescriptorType(ShaderResourceType type)
        {
            switch (type)
            {
                case ShaderResourceType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
                case ShaderResourceType::SamplerTexture: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case ShaderResourceType::Texture: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case ShaderResourceType::Image: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case ShaderResourceType::ConstantBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case ShaderResourceType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case ShaderResourceType::DynamicConstantBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case ShaderResourceType::DynamicStorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                case ShaderResourceType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                case ShaderResourceType::AccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
            }
        }

        ShaderResourceType GetShaderResourceType(VkDescriptorType type)
        {
            switch (type)
            {
                case VK_DESCRIPTOR_TYPE_SAMPLER: return ShaderResourceType::Sampler;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return ShaderResourceType::SamplerTexture;
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return ShaderResourceType::Texture;
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return ShaderResourceType::Image;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return ShaderResourceType::ConstantBuffer;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return ShaderResourceType::StorageBuffer;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return ShaderResourceType::DynamicConstantBuffer;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return ShaderResourceType::DynamicStorageBuffer;
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return ShaderResourceType::InputAttachment;
                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return ShaderResourceType::AccelerationStructure;
                default: return ShaderResourceType::Invalid;
            }
        }

        VkShaderStageFlagBits GetShaderStage(ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
                case ShaderStage::TesselationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                case ShaderStage::TesselationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
                case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
                case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
                case ShaderStage::MeshTask: return VK_SHADER_STAGE_TASK_BIT_EXT;
                case ShaderStage::MeshAssembly: return VK_SHADER_STAGE_MESH_BIT_EXT;
                case ShaderStage::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
                case ShaderStage::RayMiss: return VK_SHADER_STAGE_MISS_BIT_KHR;
                case ShaderStage::RayClosestHit: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                case ShaderStage::RayAnyHit: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                case ShaderStage::RayIntersection: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                default: return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
            }
        }

        VkPipelineBindPoint GetPipelineBindPoint(ShaderStageFlags stageFlags)
        {
            if ((stageFlags & ShaderStageFlags::StagesGraphics) != 0u)
            {
                return VK_PIPELINE_BIND_POINT_GRAPHICS;
            }

            if ((stageFlags & ShaderStageFlags::StagesCompute) != 0u)
            {
                return VK_PIPELINE_BIND_POINT_COMPUTE;
            }

            if ((stageFlags & ShaderStageFlags::StagesRayTrace) != 0u)
            {
                return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            }

            return VK_PIPELINE_BIND_POINT_MAX_ENUM;
        }

        VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples)
        {
            uint32_t bit = VK_SAMPLE_COUNT_64_BIT;

            while ((samples & bit) == 0 && bit > VK_SAMPLE_COUNT_1_BIT)
            {
                bit >>= 1;
            }

            return (VkSampleCountFlagBits)bit;
        }

        VkVertexInputRate GetInputRate(InputRate inputRate)
        {
            switch (inputRate)
            {
                case InputRate::PerVertex: return VK_VERTEX_INPUT_RATE_VERTEX;
                case InputRate::PerInstance: return VK_VERTEX_INPUT_RATE_INSTANCE;
                default: return VK_VERTEX_INPUT_RATE_MAX_ENUM;
            }
        }

        VkShaderStageFlagBits GetShaderStageFlags(ShaderStageFlags stageFlags)
        {
            uint32_t flags = 0;

            if ((stageFlags & ShaderStageFlags::Vertex) != 0)
            {
                flags |= VK_SHADER_STAGE_VERTEX_BIT;
            }

            if ((stageFlags & ShaderStageFlags::TesselationControl) != 0)
            {
                flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            }

            if ((stageFlags & ShaderStageFlags::TesselationEvaluation) != 0)
            {
                flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            }

            if ((stageFlags & ShaderStageFlags::Geometry) != 0)
            {
                flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
            }

            if ((stageFlags & ShaderStageFlags::Fragment) != 0)
            {
                flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }

            if ((stageFlags & ShaderStageFlags::Compute) != 0)
            {
                flags |= VK_SHADER_STAGE_COMPUTE_BIT;
            }

            if ((stageFlags & ShaderStageFlags::MeshTask) != 0)
            {
                flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
            }

            if ((stageFlags & ShaderStageFlags::MeshAssembly) != 0)
            {
                flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
            }

            if ((stageFlags & ShaderStageFlags::RayGeneration) != 0)
            {
                flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            }

            if ((stageFlags & ShaderStageFlags::RayMiss) != 0)
            {
                flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
            }

            if ((stageFlags & ShaderStageFlags::RayClosestHit) != 0)
            {
                flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            }

            if ((stageFlags & ShaderStageFlags::RayAnyHit) != 0)
            {
                flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            }

            if ((stageFlags & ShaderStageFlags::RayIntersection) != 0)
            {
                flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            }

            return (VkShaderStageFlagBits)flags;
        }

        VkPolygonMode GetPolygonMode(PolygonMode mode)
        {
            switch (mode)
            {
                case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
                case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
                case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
                default: return VK_POLYGON_MODE_MAX_ENUM;
            }
        }

        VkBlendOp GetBlendOp(BlendOp op)
        {
            switch (op)
            {
                case BlendOp::None: return VK_BLEND_OP_ADD;
                case BlendOp::Add: return VK_BLEND_OP_ADD;
                case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
                case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
                case BlendOp::Min: return VK_BLEND_OP_MIN;
                case BlendOp::Max: return VK_BLEND_OP_MAX;
                default: return VK_BLEND_OP_MAX_ENUM;
            }
        }

        VkBlendFactor GetBlendFactor(BlendFactor factor, VkBlendFactor fallback)
        {
            switch (factor)
            {
                case BlendFactor::None: return fallback;
                case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
                case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
                case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
                case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
                case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
                case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
                case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case BlendFactor::ConstColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case BlendFactor::OneMinusConstColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                case BlendFactor::ConstAlpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
                case BlendFactor::OneMinusConstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                default: return VK_BLEND_FACTOR_MAX_ENUM;
            }
        }

        VkLogicOp GetLogicOp(LogicOp op)
        {
            switch (op)
            {
                case LogicOp::Clear: return VK_LOGIC_OP_CLEAR;
                case LogicOp::And: return VK_LOGIC_OP_AND;
                case LogicOp::AndReverse: return VK_LOGIC_OP_AND_REVERSE;
                case LogicOp::Copy: return VK_LOGIC_OP_COPY;
                case LogicOp::AndInverted: return VK_LOGIC_OP_AND_INVERTED;
                case LogicOp::None: return VK_LOGIC_OP_NO_OP;
                case LogicOp::XOR: return VK_LOGIC_OP_XOR;
                case LogicOp::OR: return VK_LOGIC_OP_OR;
                case LogicOp::NOR: return VK_LOGIC_OP_NOR;
                case LogicOp::Equal: return VK_LOGIC_OP_EQUIVALENT;
                case LogicOp::Invert: return VK_LOGIC_OP_INVERT;
                case LogicOp::OrReverse: return VK_LOGIC_OP_OR_REVERSE;
                case LogicOp::CopyInverted: return VK_LOGIC_OP_COPY_INVERTED;
                case LogicOp::OrInverted: return VK_LOGIC_OP_OR_INVERTED;
                case LogicOp::NAND: return VK_LOGIC_OP_NAND;
                case LogicOp::Set: return VK_LOGIC_OP_SET;
                default: return VK_LOGIC_OP_MAX_ENUM;
            }
        }

        VkCullModeFlagBits GetCullMode(CullMode op)
        {
            switch (op)
            {
                case CullMode::Off: return VK_CULL_MODE_NONE;
                case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
                case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
                default: return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
            }
        }

        VkConservativeRasterizationModeEXT GetRasterMode(RasterMode mode)
        {
            switch (mode)
            {
                case RasterMode::Default: return VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
                case RasterMode::OverEstimate: return VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
                case RasterMode::UnderEstimate: return VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT;
                default: return VK_CONSERVATIVE_RASTERIZATION_MODE_MAX_ENUM_EXT;
            }
        }

        VkPrimitiveTopology GetTopology(Topology topology)
        {
            switch (topology)
            {
                case Topology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                case Topology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case Topology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                case Topology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                case Topology::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                case Topology::LineListWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
                case Topology::LineStripWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
                case Topology::TriangleListWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
                case Topology::TriangleStripWithAdjacency: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
                case Topology::PatchList: return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
                default: return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
            }
        }

        VkFrontFace GetFrontFace(FrontFace face)
        {
            switch (face)
            {
                case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
                case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                default: return VK_FRONT_FACE_MAX_ENUM;
            }
        }

        VkPipelineStageFlags GetPipelineStageFlags(VkShaderStageFlags flags)
        {
            VkPipelineStageFlags outflags = 0u;

            if (flags & VK_SHADER_STAGE_VERTEX_BIT)
            {
                outflags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            }

            if (flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
            {
                outflags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
            }

            if (flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            {
                outflags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
            }

            if (flags & VK_SHADER_STAGE_GEOMETRY_BIT)
            {
                outflags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
            }

            if (flags & VK_SHADER_STAGE_FRAGMENT_BIT)
            {
                outflags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            if (flags & VK_SHADER_STAGE_COMPUTE_BIT)
            {
                outflags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }

            if (flags & VK_SHADER_STAGE_TASK_BIT_EXT)
            {
                outflags |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT;
            }

            if (flags & VK_SHADER_STAGE_MESH_BIT_EXT)
            {
                outflags |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
            }

            if (flags &
                (VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                    VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                    VK_SHADER_STAGE_MISS_BIT_KHR |
                    VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
                    VK_SHADER_STAGE_CALLABLE_BIT_KHR))
            {
                outflags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
            }

            return outflags;
        }

        VkRayTracingShaderGroupTypeKHR GetRayTracingStageGroupType(ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::RayGeneration:
                case ShaderStage::RayMiss: return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                case ShaderStage::RayClosestHit:
                case ShaderStage::RayAnyHit: return VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                case ShaderStage::RayIntersection: return VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
                default: return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            }
        }

        bool IsReadAccess(VkAccessFlags flags)
        {
            const VkAccessFlags readMask =
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                VK_ACCESS_INDEX_READ_BIT |
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                VK_ACCESS_UNIFORM_READ_BIT |
                VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                VK_ACCESS_SHADER_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_TRANSFER_READ_BIT |
                VK_ACCESS_HOST_READ_BIT |
                VK_ACCESS_MEMORY_READ_BIT |
                VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
                VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
                VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |
                VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR |
                VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV;

            return (flags & readMask) != 0u;
        }

        bool IsWriteAccess(VkAccessFlags flags)
        {
            const VkAccessFlags writeMask =
                VK_ACCESS_SHADER_WRITE_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                VK_ACCESS_TRANSFER_WRITE_BIT |
                VK_ACCESS_HOST_WRITE_BIT |
                VK_ACCESS_MEMORY_WRITE_BIT |
                VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
                VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
                VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV;

            return (flags & writeMask) != 0u;
        }
    }

    VkResult VulkanCreateSurfaceKHR(VkInstance instance, void* nativeWindow, VkSurfaceKHR* surface)
    {
#if PK_PLATFORM_WINDOWS
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

        if (!vkCreateWin32SurfaceKHR)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.hinstance = (HINSTANCE)Platform::GetProcess();
        createInfo.hwnd = (HWND)nativeWindow;
        return vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, surface);
#else
        return VK_ERROR_UNKNOWN;
#endif
    }

    void VulkanBindExtensionMethods(VkInstance instance, bool enableDebugNames)
    {
        if (enableDebugNames)
        {
            pkfn_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
        }
        
        pkfn_vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
        pkfn_vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
        pkfn_vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
        pkfn_vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT");
        pkfn_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
        pkfn_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
        pkfn_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
        pkfn_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        pkfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        pkfn_vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

        pkfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR");
        pkfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR");
        pkfn_vkCmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)vkGetInstanceProcAddr(instance, "vkCmdSetRayTracingPipelineStackSizeKHR");
        pkfn_vkCmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysIndirectKHR");
        pkfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysKHR");
        pkfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR");
        pkfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
        pkfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR");
        pkfn_vkGetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupStackSizeKHR");
        pkfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureDeviceAddressKHR");
        pkfn_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureBuildSizesKHR");
        pkfn_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructuresKHR");
        pkfn_vkCmdCopyAccelerationStructureKHR = (PFN_vkCmdCopyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCmdCopyAccelerationStructureKHR");
        pkfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR");
        pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetInstanceProcAddr(instance, "vkCmdWriteAccelerationStructuresPropertiesKHR");
        pkfn_vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR");

        pkfn_vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksEXT");
        pkfn_vkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectEXT");
        pkfn_vkCmdDrawMeshTasksIndirectCountEXT = (PFN_vkCmdDrawMeshTasksIndirectCountEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectCountEXT");

        pkfn_vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)vkGetInstanceProcAddr(instance, "vkAcquireFullScreenExclusiveModeEXT");
        pkfn_vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)vkGetInstanceProcAddr(instance, "vkReleaseFullScreenExclusiveModeEXT");
    }

    std::vector<VkPhysicalDevice> VulkanGetPhysicalDevices(VkInstance instance)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        return devices;
    }

    std::vector<VkQueueFamilyProperties> VulkanGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        return queueFamilies;
    }

    std::vector<VkSurfaceFormatKHR> VulkanGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        std::vector<VkSurfaceFormatKHR> formats;
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        return formats;
    }

    std::vector<VkPresentModeKHR> VulkanGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        std::vector<VkPresentModeKHR> presentModes;
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

        return presentModes;
    }

    VulkanPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties2 deviceProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterizationProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };
        VkPhysicalDeviceSubgroupProperties subgroupProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };
        VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT };
        deviceProperties.pNext = &accelerationStructureProperties;
        accelerationStructureProperties.pNext = &rayTracingProperties;
        rayTracingProperties.pNext = &conservativeRasterizationProperties;
        conservativeRasterizationProperties.pNext = &subgroupProperties;
        subgroupProperties.pNext = &meshShaderProperties;

        vkGetPhysicalDeviceProperties2(device, &deviceProperties);

        VulkanPhysicalDeviceProperties returnProperties;
        returnProperties.core = deviceProperties.properties;
        returnProperties.accelerationStructure = accelerationStructureProperties;
        returnProperties.rayTracing = rayTracingProperties;
        returnProperties.conservativeRasterization = conservativeRasterizationProperties;
        returnProperties.subgroup = subgroupProperties;
        returnProperties.meshShader = meshShaderProperties;
        return returnProperties;
    }

    VulkanExclusiveFullscreenInfo VulkanGetSwapchainFullscreenInfo(const void* nativeMonitor, bool fullScreen)
    {
        VulkanExclusiveFullscreenInfo info;
        info.swapchainPNext = nullptr;

        if (fullScreen)
        {
#if PK_PLATFORM_WINDOWS
            info.win32Info = { VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT };
            info.win32Info.hmonitor = (HMONITOR)nativeMonitor;
            info.fullscreenInfo = { VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT };
            info.fullscreenInfo.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;
            info.fullscreenInfo.pNext = &info.win32Info;
            info.swapchainPNext = &info.fullscreenInfo;
#endif
        }

        return info;
    }


    bool VulkanValidateInstanceExtensions(const std::vector<const char*>* extensions)
    {
        if (extensions == nullptr || extensions->size() == 0)
        {
            return true;
        }

        auto availableCount = 0u;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, nullptr);
        auto availableExtensions = PK_STACK_ALLOC(VkExtensionProperties, availableCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, availableExtensions);

        auto foundCount = 0u;
        auto foundMask = PK_STACK_ALLOC(bool, extensions->size());
        memset(foundMask, 0, sizeof(bool) * extensions->size());

        for (auto i = 0u; i < availableCount; ++i)
        {
            auto name = availableExtensions[i].extensionName;

            for (auto j = 0u; j < extensions->size(); ++j)
            {
                if (!foundMask[j] && strcmp(extensions->at(j), name) == 0)
                {
                    foundMask[j] = true;
                    foundCount++;
                }
            }
        }

        return foundCount == extensions->size();
    }

    bool VulkanValidatePhysicalDeviceExtensions(VkPhysicalDevice device, const std::vector<const char*>* extensions)
    {
        if (extensions == nullptr || extensions->size() == 0)
        {
            return true;
        }

        auto availableCount = 0u;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableCount, nullptr);
        auto availableExtensions = PK_STACK_ALLOC(VkExtensionProperties, availableCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableCount, availableExtensions);

        auto foundCount = 0u;
        auto foundMask = PK_STACK_ALLOC(bool, extensions->size());
        memset(foundMask, 0, sizeof(bool) * extensions->size());

        for (auto i = 0u; i < availableCount; ++i)
        {
            auto name = availableExtensions[i].extensionName;

            for (auto j = 0u; j < extensions->size(); ++j)
            {
                if (!foundMask[j] && strcmp(extensions->at(j), name) == 0)
                {
                    foundMask[j] = true;
                    foundCount++;
                }
            }
        }

        return foundCount == extensions->size();
    }

    bool VulkanValidateValidationLayers(const char* const* validationLayers, const uint32_t count)
    {
        if (validationLayers == nullptr || count == 0)
        {
            return true;
        }

        uint32_t availableCount = 0u;
        vkEnumerateInstanceLayerProperties(&availableCount, nullptr);
        auto availableLayers = PK_STACK_ALLOC(VkLayerProperties, availableCount);
        vkEnumerateInstanceLayerProperties(&availableCount, availableLayers);

        auto foundCount = 0u;
        auto foundMask = PK_STACK_ALLOC(bool, count);
        memset(foundMask, 0, sizeof(bool) * count);

        for (auto i = 0u; i < availableCount; ++i)
        {
            auto name = availableLayers[i].layerName;

            for (auto j = 0u; j < count; ++j)
            {
                if (!foundMask[j] && strcmp(validationLayers[j], name) == 0)
                {
                    foundMask[j] = true;
                    foundCount++;
                }
            }
        }

        return foundCount == count;
    }

    bool VulkanIsPresentSupported(VkPhysicalDevice physicalDevice, uint32_t familyIndex, VkSurfaceKHR surface)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupport);
        return presentSupport;
    }


    void VulkanSelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* selectedDevice)
    {
        auto devices = VulkanGetPhysicalDevices(instance);
        *selectedDevice = VK_NULL_HANDLE;

        for (auto& device : devices)
        {
            auto properties = VulkanGetPhysicalDeviceProperties(device);
            auto versionMajor = VK_API_VERSION_MAJOR(properties.core.apiVersion);
            auto versionMinor = VK_API_VERSION_MINOR(properties.core.apiVersion);

            if (versionMajor < requirements.versionMajor)
            {
                continue;
            }

            if (versionMajor == requirements.versionMajor && versionMinor < requirements.versionMinor)
            {
                continue;
            }

            auto queueFamilies = VulkanGetPhysicalDeviceQueueFamilyProperties(device);
            auto extensionSupported = VulkanValidatePhysicalDeviceExtensions(device, requirements.deviceExtensions);
            auto swapChainSupported = false;

            if (extensionSupported)
            {
                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

                swapChainSupported = presentModeCount > 0 && formatCount > 0;
            }

            auto hasPresent = false;
            auto queueMask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;

            for (auto i = 0u; i < queueFamilies.size(); ++i)
            {
                auto& family = queueFamilies.at(i);
                queueMask &= ~family.queueFlags;
                hasPresent |= VulkanIsPresentSupported(device, i, surface);
            }

            if (properties.core.deviceType != requirements.deviceType ||
                !extensionSupported ||
                !swapChainSupported ||
                !hasPresent ||
                queueMask != 0u)
            {
                continue;
            }

            VulkanPhysicalDeviceFeatures features{};
            vkGetPhysicalDeviceFeatures2(device, &features.vk10);

            if (!VulkanPhysicalDeviceFeatures::CheckRequirements(requirements.features, features))
            {
                continue;
            }

            {
                PK_LOG_INFO_FUNC("from '%i' Physical Devices:", devices.size());
                PK_LOG_INFO("Name: %s", properties.core.deviceName);
                PK_LOG_INFO("Vendor: %i", properties.core.vendorID);
                PK_LOG_INFO("Device: %i", properties.core.deviceID);
                PK_LOG_INFO("Driver: %i", properties.core.driverVersion);
                PK_LOG_INFO("API VER: %i.%i", versionMajor, versionMinor);
                PK_LOG_NEWLINE();
            }

            *selectedDevice = device;
            return;
        }

        PK_THROW_ERROR("Could not find a suitable vulkan physical device!");
    }

    VkExtent2D VulkanSelectSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& desiredExtent)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }

        VkExtent2D actualExtent = desiredExtent;
        actualExtent.width = glm::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = glm::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }

    VkSurfaceFormatKHR VulkanSelectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat desiredFormat, VkColorSpaceKHR desiredColorSpace)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == desiredFormat && availableFormat.colorSpace == desiredColorSpace)
            {
                return availableFormat;
            }
        }

        return availableFormats.at(0);
    }

    VkPresentModeKHR VulkanSelectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR desiredPresentMode)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == desiredPresentMode)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }


    VkAccelerationStructureBuildSizesInfoKHR VulkanGetAccelerationBuildSizesInfo(VkDevice device, const VkAccelerationStructureBuildGeometryInfoKHR info, uint32_t primitiveCount)
    {
        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        pkfn_vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, &primitiveCount, &accelerationStructureBuildSizesInfo);
        return accelerationStructureBuildSizesInfo;
    }

    FixedString128 VulkanStr_VkQueueFlags(VkQueueFlags value) 
    {
        FixedString128 ret;
        int index = 0;

        while (value) 
        {
            if (value & 1) 
            {
                if (ret.Length() > 0u)
                {
                    ret.Append("|");
                }

                ret.Append(string_VkQueueFlagBits(static_cast<VkQueueFlagBits>(1U << index)));
            }

            ++index;
            value >>= 1;
        }
        
        if (ret.Length() == 0u)
        {
            ret.Append("VkQueueFlags(0)");
        }

        return ret;
    }

    const char* VulkanCStr_VkShaderStageFlagBits(VkShaderStageFlagBits value) { return string_VkShaderStageFlagBits(value); }
    const char* VulkanCStr_VkFormat(VkFormat value) { return string_VkFormat(value); }
    const char* VulkanCStr_VkColorSpaceKHR(VkColorSpaceKHR value) { return string_VkColorSpaceKHR(value); }
    const char* VulkanCStr_VkPresentModeKHR(VkPresentModeKHR value) { return string_VkPresentModeKHR(value); }

    VkImageSubresourceRange VulkanConvertRange(const TextureViewRange& viewRange, VkImageAspectFlags aspect)
    {
        return
        {
            aspect,                     //aspectMask
            (uint32_t)viewRange.level,  //baseMipLevel
            (uint32_t)viewRange.levels, //levelCount
            (uint32_t)viewRange.layer,  //baseArrayLayer
            (uint32_t)viewRange.layers  //layerCount
        };
    }

    TextureViewRange VulkanConvertRange(const VkImageSubresourceRange& resourceRange)
    {
        return
        {
            (uint16_t)resourceRange.baseMipLevel,           //level
            (uint16_t)resourceRange.baseArrayLayer,         //layer
            (uint16_t)(resourceRange.levelCount & 0x7FFFu), //levels
            (uint16_t)(resourceRange.layerCount & 0x7FFFu)  //layers
        };
    }

    void VulkanSetObjectDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name)
    {
        if (pkfn_vkSetDebugUtilsObjectNameEXT != nullptr)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
            nameInfo.pNext = nullptr;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = objectHandle;
            nameInfo.pObjectName = name;
            vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
            PK_LOG_RHI("%s, %s", string_VkObjectType(nameInfo.objectType), name);
        }
    }

    void VulkanAssertAPIVersion(const uint32_t major, const uint32_t minor)
    {
        uint32_t supportedApiVersion;
        VK_ASSERT_RESULT_CTX(vkEnumerateInstanceVersion(&supportedApiVersion), "Failed to query supported api version!");

        auto supportedMajor = VK_VERSION_MAJOR(supportedApiVersion);
        auto supportedMinor = VK_VERSION_MINOR(supportedApiVersion);

        if (major > supportedMajor || minor > supportedMinor)
        {
            PK_THROW_ERROR("Vulkan version %i.%i required. Your driver only supports version %i.%i", major, minor, supportedMajor, supportedMinor);
        }
    }

    void VulkanThrowError(VkResult result, const char* context)
    {
        if (context != nullptr)
        {
            PK_THROW_ERROR("%s (%s)", context, string_VkResult(result));
        }
        else
        {
            PK_THROW_ERROR("%s (%s)", context, string_VkResult(result));
        }
    }
}