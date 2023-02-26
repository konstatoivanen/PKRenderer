#include "PrecompiledHeader.h"
#include "VulkanStructs.h"
#include "VulkanUtilities.h"
#include "VulkanExtensions.h"
#include <vulkan/vk_enum_string_helper.h>

namespace PK::Rendering::VulkanRHI
{
    using namespace Structs;

    VulkanPhysicalDeviceFeatures::VulkanPhysicalDeviceFeatures()
    {
        vk10.pNext = &vk11;
        vk11.pNext = &vk12;
        vk12.pNext = &accelerationStructure;
        accelerationStructure.pNext = &rayTracingPipeline;
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
        }
    }

    VulkanImageCreateInfo::VulkanImageCreateInfo(const TextureDescriptor& descriptor, const VulkanQueueFamilies* families)
    {
        if (families)
        {
            queueFamilies = *families;
        }

        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = descriptor.samplerType == SamplerType::Sampler3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        image.format = EnumConvert::GetFormat(descriptor.format);
        image.extent = { descriptor.resolution.x, descriptor.resolution.y, descriptor.resolution.z };
        image.mipLevels = descriptor.levels;
        image.arrayLayers = descriptor.layers;
        image.samples = EnumConvert::GetSampleCountFlags(descriptor.samples);
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = 0;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.sharingMode = (descriptor.usage & TextureUsage::Concurrent) != 0 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        image.pQueueFamilyIndices = queueFamilies.indices;
        image.queueFamilyIndexCount = queueFamilies.count;
        allocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        aspect = (VkImageAspectFlagBits)0;

        if (descriptor.samplerType == SamplerType::Cubemap || 
            descriptor.samplerType == SamplerType::CubemapArray)
        {
            image.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
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
            aspect = VK_IMAGE_ASPECT_STENCIL_BIT;
            image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if ((descriptor.usage & TextureUsage::Upload) != 0)
        {
            image.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if ((descriptor.usage & TextureUsage::RTDepth) != 0)
        {
            aspect = (VkImageAspectFlagBits)((uint32_t)aspect | (uint32_t)VK_IMAGE_ASPECT_DEPTH_BIT);
            image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (aspect == (VkImageAspectFlagBits)0)
        {
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    VulkanLayoutTransition::VulkanLayoutTransition(VkImage target, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range)
    {
        image = target;
        oldLayout = srcLayout;
        newLayout = dstLayout;
        subresources = range;
        ApplyTransitionTemplate(newLayout);
    }

    VulkanLayoutTransition::VulkanLayoutTransition(VkImage target, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageLayout impostorLayout, const VkImageSubresourceRange& range)
    {
        image = target;
        oldLayout = srcLayout;
        newLayout = dstLayout;
        subresources = range;
        ApplyTransitionTemplate(impostorLayout);
    }

    void VulkanLayoutTransition::ApplyTransitionTemplate(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_GENERAL:
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dstAccessMask = 0;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                srcAccessMask = 0;
                dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                break;

            default:
                PK_THROW_ERROR("Unsupported layout transition!");
        }
    }

    VulkanImageView::VulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateImageView(device, &createInfo, nullptr, &view), "Failed to create an image view!");
        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)view, name);
    }

    VulkanImageView::~VulkanImageView()
    {
        vkDestroyImageView(device, view, nullptr);
    }
    
    VulkanFrameBuffer::VulkanFrameBuffer(VkDevice device, const VkFramebufferCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateFramebuffer(device, &createInfo, nullptr, &frameBuffer), "Failed to create framebuffer!");
    }

    VulkanFrameBuffer::~VulkanFrameBuffer()
    {
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }

    VulkanRenderPass::VulkanRenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass), "Failed to create a render pass!");
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    VulkanRawBuffer::VulkanRawBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name) :
        allocator(allocator), 
        usage(createInfo.buffer.usage), 
        capacity(createInfo.buffer.size), 
        persistentmap(createInfo.allocation.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
    {
        if ((createInfo.buffer.flags & VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT) != 0)
        {
            // No automatic memory allocation for sparse buffers. Do note that mapping a sparse buffer will throw an error.
            VK_ASSERT_RESULT_CTX(vkCreateBuffer(device, &createInfo.buffer, nullptr, &buffer), "Failed to create a buffer!");
            Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer, name);
            memory = nullptr;
        }
        else
        {
            VK_ASSERT_RESULT_CTX(vmaCreateBuffer(allocator, &createInfo.buffer, &createInfo.allocation, &buffer, &memory, &allocationInfo), "Failed to create a buffer!");
            Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_BUFFER, (uint64_t)buffer, name);
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

    void VulkanRawBuffer::SetData(const void* data, size_t size) const
    {
        memcpy(BeginMap(0ull), data, size);
        EndMap(0ull, size);
    }

    VulkanRawImage::VulkanRawImage(VkDevice device, VmaAllocator allocator, const VulkanImageCreateInfo& createInfo, const char* name) :
        allocator(allocator),
        format(createInfo.image.format),
        type(createInfo.image.imageType),
        extent(createInfo.image.extent),
        levels(createInfo.image.mipLevels),
        layers(createInfo.image.arrayLayers),
        samples(createInfo.image.samples),
        aspect(createInfo.aspect)
    {
        VK_ASSERT_RESULT_CTX(vmaCreateImage(allocator, &createInfo.image, &createInfo.allocation, &image, &memory, nullptr), "Failed to create an image!");
        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)image, name);
    }

    VulkanRawImage::~VulkanRawImage()
    {
        vmaDestroyImage(allocator, image, memory);
    }

    VulkanRawAccelerationStructure::VulkanRawAccelerationStructure(VkDevice device, 
        VmaAllocator allocator, 
        const VkAccelerationStructureGeometryKHR& geometryInfo,
        const VkAccelerationStructureBuildRangeInfoKHR& rangeInfo,
        const VkAccelerationStructureTypeKHR type,
        const char* name) :
        device(device),
        geometryInfo(geometryInfo),
        rangeInfo(rangeInfo)
    {
        auto buildSizeInfo = VulkanRHI::Utilities::VulkanGetAccelerationBuildSizesInfo(device, geometryInfo, type, rangeInfo.primitiveCount);

        scratchBufferSize = buildSizeInfo.buildScratchSize;
        rawBuffer = new VulkanRawBuffer(device, allocator, VulkanBufferCreateInfo(BufferUsage::AccelerationStructure | BufferUsage::GPUOnly, buildSizeInfo.accelerationStructureSize), name);

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.size = buildSizeInfo.accelerationStructureSize;
        createInfo.type = type;
        createInfo.buffer = rawBuffer->buffer;

        VK_ASSERT_RESULT_CTX(vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &structure), "Failed to create acceleration structure!");
        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)structure, name);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        accelerationDeviceAddressInfo.accelerationStructure = structure;
        deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
    }

    VulkanRawAccelerationStructure::~VulkanRawAccelerationStructure()
    {
        vkDestroyAccelerationStructureKHR(device, structure, nullptr);
        delete rawBuffer;
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

        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)module, name);
    }

    VulkanShaderModule::~VulkanShaderModule()
    {
        vkDestroyShaderModule(device, module, nullptr);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline, name);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateComputePipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline, name);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkRayTracingPipelineCreateInfoKHR& createInfo, const char* name) : device(device)
    {
        VK_ASSERT_RESULT_CTX(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
        Utilities::VulkanSetObjectDebugName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline, name);
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

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo, VkShaderStageFlagBits stageFlags) : device(device), stageFlags(stageFlags)
    {
        VK_ASSERT_RESULT_CTX(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout), "Failed to create a descriptor set layout!");
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

    VulkanSampler::VulkanSampler(VkDevice device, const SamplerDescriptor& descriptor) : device(device)
    {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.addressModeU = EnumConvert::GetSamplerAddressMode(descriptor.wrap[0]);
        info.addressModeV = EnumConvert::GetSamplerAddressMode(descriptor.wrap[1]);
        info.addressModeW = EnumConvert::GetSamplerAddressMode(descriptor.wrap[2]);
        info.minLod = descriptor.mipMin;
        info.maxLod = descriptor.mipMax <= 0.0f ? VK_LOD_CLAMP_NONE : descriptor.mipMax;
        info.mipLodBias = descriptor.mipBias;
        info.maxAnisotropy = descriptor.anisotropy;
        info.anisotropyEnable = descriptor.anisotropy > 0.0f ? VK_TRUE : VK_FALSE;
        info.unnormalizedCoordinates = !descriptor.normalized;
        info.borderColor = EnumConvert::GetBorderColor(descriptor.borderColor);
        info.mipmapMode = (uint32_t)descriptor.filterMin > (uint32_t)FilterMode::Bilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.compareEnable = descriptor.comparison != Comparison::Off ? VK_TRUE : VK_FALSE;
        info.compareOp = EnumConvert::GetCompareOp(descriptor.comparison);
        info.magFilter = EnumConvert::GetFilterMode(descriptor.filterMag);
        info.minFilter = EnumConvert::GetFilterMode(descriptor.filterMin);

        if (info.unnormalizedCoordinates)
        {
            info.minLod = info.maxLod = 0.0f;
        }

        VK_ASSERT_RESULT_CTX(vkCreateSampler(device, &info, nullptr, &sampler), "Failed to create a sampler!");
    }

    VulkanSampler::~VulkanSampler()
    {
        vkDestroySampler(device, sampler, nullptr);
    }
}