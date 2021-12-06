#pragma once
#include "Core/NoCopy.h"
#include "VulkanMemory.h"
#include "VulkanConstants.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Rendering::Structs;

    enum class QueueType : uint32_t
    {
        Graphics,
        Present
    };
    
    struct PhysicalDeviceRequirements
    {
        uint32_t versionMajor;
        uint32_t versionMinor;
        VkPhysicalDeviceType deviceType;
        bool alphaToOne;
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

    struct QueueFamily
    {
        uint32_t index;
        VkQueue queue;

        QueueFamily() : index(PK_INVALID_QUEUE_FAMILY), queue(VK_NULL_HANDLE) {}

        inline constexpr bool HasIndex() const { return index != PK_INVALID_QUEUE_FAMILY; }
    };

    struct QueueFamilies
    {
        QueueFamily queues[PK_QUEUE_FAMILY_COUNT];

        inline QueueFamily& operator[](const QueueType i) { return queues[(uint32_t)i]; }
        inline constexpr const QueueFamily& operator[](QueueType i) const { return queues[(uint32_t)i]; }

        inline constexpr bool HasIndices() const 
        {
            for (auto i = 0; i < PK_QUEUE_FAMILY_COUNT; ++i)
            {
                if (!queues[i].HasIndex())
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct VulkanExecutionGate
    {
        uint64_t invocationIndex = 0;
        const uint64_t* remoteInvocationIndex = nullptr;
        inline void Invalidate() { remoteInvocationIndex = nullptr; }
        inline bool IsValid() const { return remoteInvocationIndex != nullptr; }
        inline bool IsCompleted() const { return remoteInvocationIndex == nullptr || *remoteInvocationIndex != invocationIndex; }
    };

    struct VulkanDisposable : public PK::Core::NoCopy {};

    struct VulkanFence : public PK::Core::NoCopy
    {
        VulkanFence(VkDevice device, bool signaled = false);
        ~VulkanFence();
        inline VkResult GetStatus() const { return vkGetFenceStatus(device, vulkanFence); }
        const VkDevice device;
        VkFence vulkanFence;
    };

    struct VulkanSemaphore : public PK::Core::NoCopy
    {
        VulkanSemaphore(VkDevice device);
        ~VulkanSemaphore();
        const VkDevice device;
        VkSemaphore vulkanSemaphore;
    };

    struct VulkanImageView : public VulkanDisposable
    {
        VulkanImageView(VkDevice device, const VkImageViewCreateInfo& createInfo);
        ~VulkanImageView();

        const VkDevice device;
        VkImageView view;
    };

    struct VulkanFrameBuffer : public PK::Core::NoCopy
    {
        VulkanFrameBuffer(VkDevice device, const VkFramebufferCreateInfo& createInfo);
        ~VulkanFrameBuffer();

        const VkDevice device;
        VkFramebuffer frameBuffer;
    };

    struct VulkanRenderPass : public PK::Core::NoCopy
    {
        VulkanRenderPass(VkDevice device, const VkRenderPassCreateInfo& createInfo);
        ~VulkanRenderPass();

        const VkDevice device;
        VkRenderPass renderPass;
    };

    struct VulkanRawBuffer : public VulkanDisposable
    {
        VulkanRawBuffer(VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo);
        ~VulkanRawBuffer();

        void* BeginMap(size_t offset) const;
        void EndMap(size_t offset, size_t size) const;
        void SetData(const void* data, size_t size) const;

        const VmaAllocator allocator;
        const VkBufferUsageFlags usage;
        const VkDeviceSize capacity;
        VkBuffer buffer;
        VmaAllocation memory;
    };

    struct VulkanRawImage : public VulkanDisposable
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

    struct VulkanShaderModule : public VulkanDisposable
    {
        VulkanShaderModule(VkDevice device, VkShaderStageFlagBits stage, const uint32_t* spirv, size_t spirvSize);
        ~VulkanShaderModule();

        const VkDevice device;
        VkShaderModule module;
        VkPipelineShaderStageCreateInfo stageInfo;
    };

    struct VulkanPipeline : public PK::Core::NoCopy 
    {
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo);
        ~VulkanPipeline();

        const VkDevice device;
        VkPipeline pipeline;
    };

    struct VulkanPipelineLayout : public PK::Core::NoCopy
    {
        VulkanPipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& createInfo);
        ~VulkanPipelineLayout();

        const VkDevice device;
        VkPipelineLayout layout;
    };

    struct VulkanDescriptorSetLayout : public PK::Core::NoCopy
    {
        VulkanDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo);
        ~VulkanDescriptorSetLayout();

        const VkDevice device;
        VkDescriptorSetLayout layout;
    };

    struct VulkanDescriptorPool : public PK::Core::NoCopy
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
        mutable VulkanExecutionGate executionGate;
    };

    struct VulkanSampler : public PK::Core::NoCopy
    {
        VulkanSampler(VkDevice device, const SamplerDescriptor& descriptor);
        ~VulkanSampler();

        const VkDevice device;
        VkSampler sampler;
    };

    struct VulkanRawCommandBuffer : public PK::Core::NoCopy
    {
        VulkanRawCommandBuffer() {}
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        uint64_t invocationIndex = 0;

        inline bool IsActive() const { return commandBuffer != VK_NULL_HANDLE; }
        inline VulkanExecutionGate GetOnCompleteGate() const { return { invocationIndex, &invocationIndex }; }

        void BeginRenderPass(const VkRenderPassBeginInfo& beginInfo);
        void EndRenderPass() const;
        void BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const;
        void SetViewPorts(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const;
        void SetViewPort(const VkViewport& pViewport) const;
        void SetViewPort(const VkRect2D& rect, float minDepth, float maxDepth) const;
        void SetScissors(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) const;
        void SetScissor(const VkRect2D& pScissor) const;
        void SetVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const;
        void SetVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const std::initializer_list<VkDeviceSize> pOffsets) const;
        void BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const;
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, const VkExtent3D& extent, uint32_t level, uint32_t layer) const;
        void PipelineBarrier(VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers) const;

        void TransitionImageLayout(const VulkanLayoutTransition& transition) const;

        void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint,
            const VulkanPipelineLayout* layout,
            uint32_t firstSet,
            uint32_t descriptorSetCount,
            const VulkanDescriptorSet** pDescriptorSets,
            uint32_t dynamicOffsetCount,
            const uint32_t* pDynamicOffsets) const;
    };

    struct VulkanBindHandle
    {
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize bufferRange = 0ull;
        VkDeviceSize bufferOffset = 0ull;
        const BufferLayout* bufferLayout = nullptr;
        VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    };

    struct VulkanRenderTarget : public PK::Core::NoCopy
    {
        VulkanRenderTarget(VkImageView view,
                           VkImage image,
                           VkImageLayout layout,
                           VkImageAspectFlagBits aspect,
                           VkFormat format,
                           VkExtent3D extent,
                           uint32_t samples,
                           uint32_t layers) : 
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
        const uint32_t samples;
        const uint32_t layers;
    };
}