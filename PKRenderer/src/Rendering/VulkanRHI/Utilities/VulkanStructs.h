#pragma once
#include "Utilities/IDObject.h"
#include "vulkan/vulkan.h"
#include "VulkanMemory.h"
#include "Rendering/Structs/ExecutionGate.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Rendering::Structs;
    using namespace PK::Utilities;

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

    struct PhysicalDeviceRequirements
    {
        uint32_t versionMajor;
        uint32_t versionMinor;
        VkPhysicalDeviceType deviceType;
        VkPhysicalDeviceFeatures features;

        const std::vector<const char*>* deviceExtensions;
    };

    struct VulkanBufferCreateInfo
    {
        VulkanBufferCreateInfo() {};
        VulkanBufferCreateInfo(BufferUsage usage, size_t size);

        VkBufferCreateInfo buffer = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        VmaAllocationCreateInfo allocation = {};
    };

    struct VulkanImageCreateInfo
    {
        VulkanImageCreateInfo() {};
        VulkanImageCreateInfo(const TextureDescriptor& descriptor);

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

    struct IVulkanDisposable : public IDObject 
    {
        virtual ~IVulkanDisposable() = 0 {};
    };

    struct VulkanFence : public NoCopy
    {
        VulkanFence(VkDevice device, bool signaled = false);
        ~VulkanFence();
        inline VkResult GetStatus() const { return vkGetFenceStatus(device, vulkanFence); }
        const VkDevice device;
        VkFence vulkanFence;
    };

    struct VulkanSemaphore : public NoCopy
    {
        VulkanSemaphore(VkDevice device);
        ~VulkanSemaphore();
        const VkDevice device;
        VkSemaphore vulkanSemaphore;
    };

    struct VulkanImageView : public IVulkanDisposable
    {
        VulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo);
        ~VulkanImageView();

        const VkDevice device;
        VkImageView view;
    };

    struct VulkanFrameBuffer : public NoCopy
    {
        VulkanFrameBuffer(VkDevice device, const VkFramebufferCreateInfo& createInfo);
        ~VulkanFrameBuffer();

        const VkDevice device;
        VkFramebuffer frameBuffer;
    };

    struct VulkanRenderPass : public NoCopy
    {
        VulkanRenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo);
        ~VulkanRenderPass();

        const VkDevice device;
        VkRenderPass renderPass;
    };

    struct VulkanRawBuffer : public IVulkanDisposable
    {
        VulkanRawBuffer(VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo);
        ~VulkanRawBuffer();

        void* BeginMap(size_t offset) const;
        void EndMap(size_t offset, size_t size) const;
        void SetData(const void* data, size_t size) const;

        const bool persistentmap;
        const VmaAllocator allocator;
        const VkBufferUsageFlags usage;
        const VkDeviceSize capacity;
        VkBuffer buffer;
        VmaAllocation memory;
        VmaAllocationInfo allocationInfo;
    };

    struct VulkanRawImage : public IVulkanDisposable
    {
        VulkanRawImage(VmaAllocator allocator, const VulkanImageCreateInfo& createInfo);
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

    struct VulkanShaderModule : public IVulkanDisposable
    {
        VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t spirvSize);
        ~VulkanShaderModule();

        const VkDevice device;
        VkShaderModule module;
        VkPipelineShaderStageCreateInfo stageInfo;
    };

    struct VulkanPipeline : public NoCopy 
    {
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo);
        ~VulkanPipeline();

        const VkDevice device;
        VkPipeline pipeline;
    };

    struct VulkanPipelineLayout : public NoCopy
    {
        VulkanPipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo);
        ~VulkanPipelineLayout();

        const VkDevice device;
        VkPipelineLayout layout;
    };

    struct VulkanDescriptorSetLayout : public NoCopy
    {
        VulkanDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo, VkShaderStageFlagBits stageFlags);
        ~VulkanDescriptorSetLayout();

        const VkDevice device;
        VkDescriptorSetLayout layout;
        VkShaderStageFlagBits stageFlags;
    };

    struct VulkanDescriptorPool : public NoCopy
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
        mutable ExecutionGate executionGate;
    };

    struct VulkanSampler : public NoCopy
    {
        VulkanSampler(VkDevice device, const SamplerDescriptor& descriptor);
        ~VulkanSampler();

        const VkDevice device;
        VkSampler sampler;
    };

    struct VulkanBindHandle : IDObject
    {
        union
        {
            VkImageView imageView = VK_NULL_HANDLE;
            VkBuffer buffer;
        };

        union
        {
            VkSampler sampler = VK_NULL_HANDLE;
            const BufferLayout* bufferLayout;
        };

        union
        {
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkVertexInputRate inputRate;
        };

        VkDeviceSize bufferRange = 0ull;
        VkDeviceSize bufferOffset = 0ull;
    };

    struct VulkanRenderTarget : public NoCopy
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