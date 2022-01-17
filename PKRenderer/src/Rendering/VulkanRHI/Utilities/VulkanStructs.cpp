#include "PrecompiledHeader.h"
#include "VulkanStructs.h"
#include "VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI
{
    VulkanBufferCreateInfo::VulkanBufferCreateInfo(BufferUsage usage, size_t size)
    {
        buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer.size = size;
        buffer.usage = 0u;

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
            buffer.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }

        if ((usage & BufferUsage::Index) != 0)
        {
            buffer.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
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

        if ((usage & BufferUsage::PersistentStage) != 0)
        {
            allocation.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
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

    VulkanImageCreateInfo::VulkanImageCreateInfo(const TextureDescriptor& descriptor)
    {
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

    VulkanFence::VulkanFence(VkDevice device, bool signaled) : device(device)
    {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        if (signaled) 
        {
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VK_ASSERT_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &vulkanFence));
    }
    
    VulkanFence::~VulkanFence()
    {
        vkDestroyFence(device, vulkanFence, nullptr);
    }
    
    VulkanSemaphore::VulkanSemaphore(VkDevice device) : device(device)
    {
        VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_ASSERT_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &vulkanSemaphore));
    }
    
    VulkanSemaphore::~VulkanSemaphore()
    {
        vkDestroySemaphore(device, vulkanSemaphore, nullptr);
    }

    VulkanImageView::VulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo) : device(device)
    {
        PK_LOG_VERBOSE("VK ALLOC: Image View");
        VK_ASSERT_RESULT_CTX(vkCreateImageView(device, &createInfo, nullptr, &view), "Failed to create an image view!");
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

    VulkanRawBuffer::VulkanRawBuffer(VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo) : 
        allocator(allocator), 
        usage(createInfo.buffer.usage), 
        capacity(createInfo.buffer.size), 
        persistentmap(createInfo.allocation.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
    {
        PK_LOG_VERBOSE("VK ALLOC: Buffer");
        VK_ASSERT_RESULT_CTX(vmaCreateBuffer(allocator, &createInfo.buffer, &createInfo.allocation, &buffer, &memory, &allocationInfo), "Failed to create a buffer!");
    }
    
    VulkanRawBuffer::~VulkanRawBuffer()
    {
        vmaDestroyBuffer(allocator, buffer, memory);
    }

    void* VulkanRawBuffer::BeginMap(size_t offset) const
    {
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
        if (!persistentmap)
        {
            vmaUnmapMemory(allocator, memory);
        }

        vmaFlushAllocation(allocator, memory, offset, size);
    }

    void VulkanRawBuffer::SetData(const void* data, size_t size) const
    {
        memcpy(BeginMap(0ull), data, size);
        EndMap(0ull, size);
    }

    VulkanRawImage::VulkanRawImage(VmaAllocator allocator, const VulkanImageCreateInfo& createInfo) : 
        allocator(allocator),
        format(createInfo.image.format),
        type(createInfo.image.imageType),
        extent(createInfo.image.extent),
        levels(createInfo.image.mipLevels),
        layers(createInfo.image.arrayLayers),
        samples(createInfo.image.samples),
        aspect(createInfo.aspect)
    {
        PK_LOG_VERBOSE("VK ALLOC: Image");
        VK_ASSERT_RESULT_CTX(vmaCreateImage(allocator, &createInfo.image, &createInfo.allocation, &image, &memory, nullptr), "Failed to create an image!");
    }

    VulkanRawImage::~VulkanRawImage()
    {
        vmaDestroyImage(allocator, image, memory);
    }

    VulkanShaderModule::VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t sprivSize) : device(device)
    {
        VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = sprivSize;
        createInfo.pCode = spirv;

        VK_ASSERT_RESULT_CTX(vkCreateShaderModule(device, &createInfo, nullptr, &module), "Failed to create shader module!");

        stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageInfo.stage = stage;
        stageInfo.module = module;
        stageInfo.pName = "main";
    }

    VulkanShaderModule::~VulkanShaderModule()
    {
        vkDestroyShaderModule(device, module, nullptr);
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo) : device(device)
    {
        PK_LOG_VERBOSE("VK ALLOC: Graphics Pipeline");
        VK_ASSERT_RESULT_CTX(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
    }

    VulkanPipeline::VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo) : device(device)
    {
        PK_LOG_VERBOSE("VK ALLOC: Compute Pipeline");
        VK_ASSERT_RESULT_CTX(vkCreateComputePipelines(device, pipelineCache, 1, &createInfo, nullptr, &pipeline), "failed to create a graphics pipeline!");
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
        info.mipmapMode = (uint32_t)descriptor.filter > (uint32_t)FilterMode::Bilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.compareEnable = descriptor.comparison != Comparison::Off ? VK_TRUE : VK_FALSE;
        info.compareOp = EnumConvert::GetCompareOp(descriptor.comparison);
        info.magFilter = EnumConvert::GetFilterMode(descriptor.filter);
        info.minFilter = EnumConvert::GetFilterMode(descriptor.filter);

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