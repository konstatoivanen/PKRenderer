#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/RHI/Driver.h"
#include "Rendering/RHI/Disposer.h"
#include "Rendering/RHI/Vulkan/VulkanCommon.h"
#include "Rendering/RHI/Vulkan/Services/VulkanDescriptorCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanStagingBufferCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanCommandBufferPool.h"
#include "Rendering/RHI/Vulkan/Services/VulkanFrameBufferCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanLayoutCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanBarrierHandler.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanQueue.h"

namespace PK::Rendering::RHI::Vulkan
{
    struct VulkanContextProperties
    {
        std::string appName;
        std::string workingDirectory;
        uint64_t garbagePruneDelay;
        uint32_t minApiVersionMajor;
        uint32_t minApiVersionMinor;
        const std::vector<const char*>* validationLayers;
        const std::vector<const char*>* contextualInstanceExtensions;
        const std::vector<const char*>* contextualDeviceExtensions;

        VulkanContextProperties(
            const std::string& appName = "Vulkan Engine",
            const std::string& workingDirectory = "",
            uint64_t garbagePruneDelay = 32ull,
            uint32_t minApiVersionMajor = 1,
            uint32_t minApiVersionMinor = 2,
            const std::vector<const char*>* validationLayers = nullptr,
            const std::vector<const char*>* contextualInstanceExtensions = nullptr,
            const std::vector<const char*>* contextualDeviceExtensions = nullptr) :
            appName(appName),
            workingDirectory(workingDirectory),
            garbagePruneDelay(garbagePruneDelay),
            minApiVersionMajor(minApiVersionMajor),
            minApiVersionMinor(minApiVersionMinor),
            validationLayers(validationLayers),
            contextualInstanceExtensions(contextualInstanceExtensions),
            contextualDeviceExtensions(contextualDeviceExtensions)
        {
        }
    };

    struct VulkanDriver : public Driver
    {
        VulkanDriver(const VulkanContextProperties& properties);
        ~VulkanDriver();

        APIType GetAPI() const final { return APIType::Vulkan; }
        RHI::Objects::QueueSet* GetQueues() const final { return queues.get(); }
        std::string GetDriverHeader() const;
        DriverMemoryInfo GetMemoryInfo() const final;
        size_t GetBufferOffsetAlignment(BufferUsage usage) const final;

        void SetBuffer(PK::Utilities::NameID name, RHI::Objects::Buffer* buffer, const IndexRange& range) final;
        void SetTexture(PK::Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range) final;
        void SetBufferArray(PK::Utilities::NameID name, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray) final;
        void SetTextureArray(PK::Utilities::NameID name, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray) final;
        void SetImage(PK::Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range) final;
        void SetSampler(PK::Utilities::NameID name, const SamplerDescriptor& sampler) final;
        void SetAccelerationStructure(PK::Utilities::NameID name, RHI::Objects::AccelerationStructure* structure) final;
        void SetConstant(PK::Utilities::NameID name, const void* data, uint32_t size) final;
        void SetKeyword(PK::Utilities::NameID name, bool value) final;

        void WaitForIdle() const final { vkDeviceWaitIdle(device); }
        void GC() final;

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);

        void DisposePooledImageView(VulkanImageView* view, const PK::Utilities::FenceRef& fence) const;
        void DisposePooledImage(VulkanRawImage* image, const PK::Utilities::FenceRef& fence) const;
        void DisposePooledBuffer(VulkanRawBuffer* buffer, const PK::Utilities::FenceRef& fence) const;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VmaAllocator allocator;
        VulkanContextProperties properties;
        VulkanPhysicalDeviceProperties physicalDeviceProperties;
        uint32_t apiVersion;

        PK::Utilities::Scope<Objects::VulkanQueueSet> queues;
        PK::Utilities::Scope<Services::VulkanFrameBufferCache> frameBufferCache;
        PK::Utilities::Scope<Services::VulkanStagingBufferCache> stagingBufferCache;
        PK::Utilities::Scope<Services::VulkanDescriptorCache> descriptorCache;
        PK::Utilities::Scope<Services::VulkanPipelineCache> pipelineCache;
        PK::Utilities::Scope<Services::VulkanSamplerCache> samplerCache;
        PK::Utilities::Scope<Services::VulkanLayoutCache> layoutCache;
        PK::Utilities::Scope<Disposer> disposer;
        mutable PK::Utilities::FixedPool<VulkanBindHandle, 4096> bindhandlePool;
        mutable PK::Utilities::FixedPool<VulkanImageView, 2048> imageViewPool;
        mutable PK::Utilities::FixedPool<VulkanRawImage, 2048> imagePool;
        mutable PK::Utilities::FixedPool<VulkanRawBuffer, 2048> bufferPool;
    };
}