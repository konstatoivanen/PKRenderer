#pragma once
#include "Utilities/VersionedObject.h"
#include "vulkan/vulkan.h"
#include "VulkanMemory.h"
#include "Rendering/Structs/ExecutionGate.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::VulkanRHI
{
    constexpr const static uint32_t PK_QUEUE_FAMILY_COUNT = 3;
    constexpr const static uint32_t PK_INVALID_QUEUE_FAMILY = 0xFFFFFFFF;

    enum class QueueType : uint32_t
    {
        Graphics,
        Compute,
        Present
    };
    
    typedef struct QueueFamilies 
    {
        uint32_t indices[3] = 
        { 
            PK_INVALID_QUEUE_FAMILY, 
            PK_INVALID_QUEUE_FAMILY, 
            PK_INVALID_QUEUE_FAMILY 
        }; 

        constexpr uint32_t operator[](const QueueType i) const { return indices[(uint32_t)i]; }
        uint32_t& operator[](const QueueType i) { return indices[(uint32_t)i]; }

    } QueueFamilies;

    struct VulkanPhysicalDeviceFeatures
    {
        VkPhysicalDeviceFeatures2 vk10{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan11Features vk11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features vk12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        
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
    };

    struct VulkanBufferCreateInfo
    {
        VulkanBufferCreateInfo() {};
        VulkanBufferCreateInfo(Structs::BufferUsage usage, size_t size);

        VkBufferCreateInfo buffer { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        VmaAllocationCreateInfo allocation {};
    };

    struct VulkanImageCreateInfo
    {
        VulkanImageCreateInfo() {};
        VulkanImageCreateInfo(const Structs::TextureDescriptor& descriptor);

        VkImageCreateInfo image = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        VmaAllocationCreateInfo allocation = {};
        VkImageAspectFlagBits aspect = (VkImageAspectFlagBits)0;
    };

    struct VulkanLayoutTransition 
    {
        VulkanLayoutTransition() {}
        VulkanLayoutTransition(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range);
        VulkanLayoutTransition(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageLayout impostorLayout, const VkImageSubresourceRange& range);

        void ApplyTransitionTemplate(VkImageLayout layout);

        VkImage image = nullptr;
        VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageSubresourceRange subresources = {};
        VkPipelineStageFlags srcStage = 0;
        VkAccessFlags srcAccessMask = 0;
        VkPipelineStageFlags dstStage = 0;
        VkAccessFlags dstAccessMask = 0;
    };

    struct IVulkanDisposable : public PK::Utilities::VersionedObject 
    {
        virtual ~IVulkanDisposable() = 0 {};
    };

    struct VulkanFence : public PK::Utilities::NoCopy
    {
        VulkanFence(VkDevice device, bool signaled = false);
        ~VulkanFence();

        const VkDevice device;
        VkFence vulkanFence;
    };

    struct VulkanSemaphore : public PK::Utilities::NoCopy
    {
        VulkanSemaphore(VkDevice device);
        ~VulkanSemaphore();

        const VkDevice device;
        VkSemaphore vulkanSemaphore;
    };

    struct VulkanImageView : public IVulkanDisposable
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

    struct VulkanRawBuffer : public IVulkanDisposable
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

    struct VulkanRawImage : public IVulkanDisposable
    {
        VulkanRawImage(VkDevice device, VmaAllocator allocator, const VulkanImageCreateInfo& createInfo, const char* name);
        ~VulkanRawImage();

        const VmaAllocator allocator;
        VkImage image;
        VmaAllocation memory;
        VkImageAspectFlagBits aspect;
        VkSampleCountFlagBits samples;
        VkFormat format;
        VkImageType type;
        VkExtent3D extent;
        uint32_t levels;
        uint32_t layers;
    };

    struct VulkanRawAccelerationStructure : public IVulkanDisposable
    {
        VulkanRawAccelerationStructure(VkDevice device, 
            VmaAllocator allocator, 
            const VkAccelerationStructureGeometryKHR& geometryInfo,
            const VkAccelerationStructureBuildRangeInfoKHR& rangeInfo,
            const VkAccelerationStructureTypeKHR type,
            const char* name);
        ~VulkanRawAccelerationStructure();

        const VkDevice device;
        VulkanRawBuffer* rawBuffer;
        VkDeviceSize scratchBufferSize;
        VkDeviceAddress deviceAddress;
        VkAccelerationStructureKHR structure;
        VkAccelerationStructureGeometryKHR geometryInfo;
        VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
    };

    struct VulkanShaderModule : public IVulkanDisposable
    {
        VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t spirvSize, const char* name);
        ~VulkanShaderModule();

        const VkDevice device;
        VkShaderModule module;
        VkPipelineShaderStageCreateInfo stageInfo;
    };

    struct VulkanPipeline : public PK::Utilities::NoCopy
    {
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkRayTracingPipelineCreateInfoKHR& createInfo);
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
        mutable Rendering::Structs::ExecutionGate executionGate;
    };

    struct VulkanSampler : public PK::Utilities::NoCopy
    {
        VulkanSampler(VkDevice device, const Rendering::Structs::SamplerDescriptor& descriptor);
        ~VulkanSampler();

        const VkDevice device;
        VkSampler sampler;
    };

    struct VulkanBindHandle : PK::Utilities::VersionedObject
    {
        union
        {
            VkImageView imageView = VK_NULL_HANDLE;
            VkBuffer buffer;
            VkAccelerationStructureKHR accelerationStructure;
        };

        union
        {
            VkSampler sampler = VK_NULL_HANDLE;
            const Rendering::Structs::BufferLayout* bufferLayout;
        };

        union
        {
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkVertexInputRate inputRate;
        };

        VkDeviceSize bufferRange = 0ull;
        VkDeviceSize bufferOffset = 0ull;
    };

    struct VulkanRenderTarget : public PK::Utilities::NoCopy
    {
        VulkanRenderTarget(VkImageView view,
                           VkImage image,
                           VkImageLayout layout,
                           VkImageAspectFlagBits aspect,
                           VkFormat format,
                           VkExtent3D extent,
                           uint16_t samples,
                           uint16_t layers) : 
            view(view), 
            image(image),
            layout(layout), 
            aspect(aspect),
            format(format), 
            extent(extent), 
            samples(samples), 
            layers(layers) {}

        VkImageView view;
        VkImage image;
        const VkImageLayout layout;
        const VkImageAspectFlagBits aspect;
        const VkFormat format;
        const VkExtent3D extent;
        const uint16_t samples;
        const uint16_t layers;
    };
}