#pragma once
#include "Utilities/VersionedObject.h"
#include "vulkan/vulkan.h"
#include "VulkanMemory.h"
#include "Rendering/Structs/FenceRef.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/Structs/Layout.h"
#include "Rendering/Services/Disposer.h"

namespace PK::Rendering::VulkanRHI
{
    struct VulkanQueueFamilies
    {
        uint32_t indices[(uint32_t)Structs::QueueType::MaxCount]{};
        uint32_t count;
    };

    struct VulkanBarrierInfo
    {
        uint16_t srcQueueFamily = 0u;
        uint16_t dstQueueFamily = 0u;
        VkPipelineStageFlags srcStageMask = 0u;
        VkPipelineStageFlags dstStageMask = 0u;
        VkDependencyFlags dependencyFlags = 0u;
        uint32_t memoryBarrierCount = 0u;
        const VkMemoryBarrier* pMemoryBarriers = nullptr;
        uint32_t bufferMemoryBarrierCount = 0u;
        const VkBufferMemoryBarrier* pBufferMemoryBarriers = nullptr;
        uint32_t imageMemoryBarrierCount = 0u;
        const VkImageMemoryBarrier* pImageMemoryBarriers = nullptr;
    };

    struct VulkanPhysicalDeviceFeatures
    {
        VkPhysicalDeviceFeatures2 vk10{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan11Features vk11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features vk12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceVulkan13Features vk13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
        VulkanPhysicalDeviceFeatures();
    };

    struct VulkanPhysicalDeviceRequirements
    {
        uint32_t versionMajor;
        uint32_t versionMinor;
        VkPhysicalDeviceType deviceType;
        VulkanPhysicalDeviceFeatures features;

        const std::vector<const char*>* deviceExtensions;
    };

    struct VulkanPhysicalDeviceProperties
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties;
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties;
        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterizationProperties;
    };

    struct VulkanTimelineSemaphore
    {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        uint64_t counter = 0ull;
    };

    struct VulkanBufferCreateInfo
    {
        VulkanBufferCreateInfo() {};
        VulkanBufferCreateInfo(Structs::BufferUsage usage, size_t size, const VulkanQueueFamilies* families = nullptr);

        VkBufferCreateInfo buffer { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        VmaAllocationCreateInfo allocation {};
        VulkanQueueFamilies queueFamilies{};
    };

    struct VulkanImageCreateInfo
    {
        VulkanImageCreateInfo() {};
        VulkanImageCreateInfo(const Structs::TextureDescriptor& descriptor, const VulkanQueueFamilies* families = nullptr);

        VkImageCreateInfo image = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        VmaAllocationCreateInfo allocation = {};
        VkImageAspectFlagBits aspect = (VkImageAspectFlagBits)0;
        VulkanQueueFamilies queueFamilies{};
    };

    struct VulkanImageView : public PK::Utilities::VersionedObject
    {
        VulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo, const char* name);
        ~VulkanImageView();

        const VkDevice device;
        VkImageView view;
    };

    struct VulkanFrameBuffer : public PK::Utilities::NoCopy
    {
        VulkanFrameBuffer(VkDevice device, const VkFramebufferCreateInfo& createInfo);
        ~VulkanFrameBuffer();

        const VkDevice device;
        VkFramebuffer frameBuffer;
    };

    struct VulkanRenderPass : public PK::Utilities::NoCopy
    {
        VulkanRenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo);
        ~VulkanRenderPass();

        const VkDevice device;
        VkRenderPass renderPass;
    };

    struct VulkanRawBuffer : public PK::Utilities::VersionedObject
    {
        VulkanRawBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name);
        ~VulkanRawBuffer();

        void Invalidate(size_t offset, size_t size) const;
        void* BeginMap(size_t offset) const;
        void EndMap(size_t offset, size_t size) const;
        void SetData(const void* data, size_t size) const;

        const bool persistentmap;
        const VmaAllocator allocator;
        const VkBufferUsageFlags usage;
        const VkDeviceSize capacity;
        VkDeviceAddress deviceAddress;
        VkBuffer buffer;
        VmaAllocation memory;
        VmaAllocationInfo allocationInfo{};
    };

    struct VulkanRawImage : public PK::Utilities::VersionedObject
    {
        VulkanRawImage(VkDevice device, VmaAllocator allocator, const VulkanImageCreateInfo& createInfo, const char* name);
        ~VulkanRawImage();

        const VmaAllocator allocator;
        VkImage image;
        VkImage imageAlias;
        VmaAllocation memory;
        VkImageAspectFlagBits aspect;
        VkSampleCountFlagBits samples;
        VkFormat format;
        VkImageType type;
        VkExtent3D extent;
        uint32_t levels;
        uint32_t layers;
    };

    struct VulkanRawAccelerationStructure : public PK::Utilities::VersionedObject
    {
        VulkanRawAccelerationStructure(VkDevice device, const VkAccelerationStructureCreateInfoKHR& createInfo, const char* name);
        ~VulkanRawAccelerationStructure();
        const VkDevice device;
        VkAccelerationStructureKHR structure;
        VkDeviceAddress deviceAddress;
    };

    struct VulkanShaderModule : public PK::Utilities::VersionedObject
    {
        VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t spirvSize, const char* name);
        ~VulkanShaderModule();

        const VkDevice device;
        VkShaderModule module;
        VkPipelineShaderStageCreateInfo stageInfo;
    };

    struct VulkanPipeline : public PK::Utilities::NoCopy
    {
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo, const char* name);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo, const char* name);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkRayTracingPipelineCreateInfoKHR& createInfo, const char* name);
        ~VulkanPipeline();

        const VkDevice device;
        VkPipeline pipeline;
    };

    struct VulkanPipelineLayout : public PK::Utilities::NoCopy
    {
        VulkanPipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo);
        ~VulkanPipelineLayout();

        const VkDevice device;
        VkPipelineLayout layout;
    };

    struct VulkanDescriptorSetLayout : public PK::Utilities::NoCopy
    {
        VulkanDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo, VkShaderStageFlagBits stageFlags);
        ~VulkanDescriptorSetLayout();

        const VkDevice device;
        VkDescriptorSetLayout layout;
        VkShaderStageFlagBits stageFlags;
    };

    struct VulkanDescriptorPool : public PK::Utilities::NoCopy
    {
        VulkanDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& createInfo);
        ~VulkanDescriptorPool();

        void Reset(VkDescriptorPoolResetFlags flags) { vkResetDescriptorPool(device, pool, flags); }

        const VkDevice device;
        VkDescriptorPool pool;
    };

    struct VulkanDescriptorSet
    {
        VkDescriptorSet set;
        uint64_t pruneTick;
        mutable Rendering::Structs::FenceRef fence;
    };

    struct VulkanSampler : public PK::Utilities::NoCopy
    {
        VulkanSampler(VkDevice device, const Rendering::Structs::SamplerDescriptor& descriptor);
        ~VulkanSampler();

        const VkDevice device;
        VkSampler sampler;
    };

    struct VulkanQueryPool : public PK::Utilities::NoCopy
    {
        VulkanQueryPool(VkDevice device, VkQueryType type, uint32_t size);
        ~VulkanQueryPool();

        bool TryGetResults(void* outBuffer, size_t stride, VkQueryResultFlagBits flags);
        uint32_t AddQuery(const Rendering::Structs::FenceRef& fence);

        const VkDevice device;
        const uint32_t size;
        const VkQueryType type;
        Rendering::Structs::FenceRef lastQueryFence;
        uint32_t count;
        VkQueryPool pool;
    };

    struct VulkanBindHandle : PK::Utilities::VersionedObject
    {
        union
        {
            struct Image
            {
                VkImage image = VK_NULL_HANDLE;
                VkImage alias = VK_NULL_HANDLE;
                VkImageView view = VK_NULL_HANDLE;
                VkSampler sampler = VK_NULL_HANDLE;
                VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
                VkFormat format = VK_FORMAT_UNDEFINED;
                VkExtent3D extent = { 0u, 0u, 0u };
                VkImageSubresourceRange range = { VK_IMAGE_ASPECT_NONE, 0u, VK_REMAINING_MIP_LEVELS, 0u, VK_REMAINING_ARRAY_LAYERS };
                uint16_t samples = 1u;
            } 
            image;

            struct Buffer
            {
                VkBuffer buffer;
                const Rendering::Structs::BufferLayout* layout;
                VkVertexInputRate inputRate;
                VkDeviceSize offset;
                VkDeviceSize range;
            } 
            buffer;

            struct Acceleration
            {
                VkAccelerationStructureKHR structure;
            } 
            acceleration;
        };

        bool isConcurrent = false;
        bool isTracked = true;

        VulkanBindHandle() : image{}{};
    };
}