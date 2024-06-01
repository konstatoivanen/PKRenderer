#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/Disposer.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/Services/VulkanDescriptorCache.h"
#include "Core/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Core/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Core/RHI/Vulkan/Services/VulkanStagingBufferCache.h"
#include "Core/RHI/Vulkan/Services/VulkanCommandBufferPool.h"
#include "Core/RHI/Vulkan/Services/VulkanFrameBufferCache.h"
#include "Core/RHI/Vulkan/Services/VulkanLayoutCache.h"
#include "Core/RHI/Vulkan/Services/VulkanBarrierHandler.h"
#include "Core/RHI/Vulkan/VulkanQueue.h"

namespace PK
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

    struct VulkanDriver : public RHIDriver
    {
        VulkanDriver(const VulkanContextProperties& properties);
        ~VulkanDriver();

        RHIAPI GetAPI() const final { return RHIAPI::Vulkan; }
        RHIQueueSet* GetQueues() const final { return queues.get(); }
        std::string GetDriverHeader() const;
        RHIDriverMemoryInfo GetMemoryInfo() const final;
        size_t GetBufferOffsetAlignment(BufferUsage usage) const final;
        BuiltInResources* GetBuiltInResources() final { return builtInResources; }
        PropertyBlock* GetResourceState() final { return &globalResources; }

        RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) final;
        RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) final;
        RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) final;
        RHITextureBindArrayRef CreateTextureBindArray(size_t capacity) final;
        RHIBufferBindArrayRef CreateBufferBindArray(size_t capacity) final;
        RHIShaderScope CreateShader(void* base, PK::Assets::Shader::PKShaderVariant* pVariant, const char* name) final;
        RHIWindowScope CreateWindowScope(const WindowDescriptor& descriptor) final;

        void SetBuffer(NameID name, RHIBuffer* buffer, const BufferIndexRange& range) final;
        void SetTexture(NameID name, RHITexture* texture, const TextureViewRange& range) final;
        void SetBufferArray(NameID name, RHIBindArray<RHIBuffer>* bufferArray) final;
        void SetTextureArray(NameID name, RHIBindArray<RHITexture>* textureArray) final;
        void SetImage(NameID name, RHITexture* texture, const TextureViewRange& range) final;
        void SetSampler(NameID name, const SamplerDescriptor& sampler) final;
        void SetAccelerationStructure(NameID name, RHIAccelerationStructure* structure) final;
        void SetConstant(NameID name, const void* data, uint32_t size) final;
        void SetKeyword(NameID name, bool value) final;

        void WaitForIdle() const final { vkDeviceWaitIdle(device); }
        void GC() final;

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);

        void DisposePooledImageView(VulkanImageView* view, const FenceRef& fence) const;
        void DisposePooledImage(VulkanRawImage* image, const FenceRef& fence) const;
        void DisposePooledBuffer(VulkanRawBuffer* buffer, const FenceRef& fence) const;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VmaAllocator allocator;
        VulkanContextProperties properties;
        VulkanPhysicalDeviceProperties physicalDeviceProperties;
        uint32_t apiVersion;

        Scope<VulkanQueueSet> queues;
        Scope<VulkanFrameBufferCache> frameBufferCache;
        Scope<VulkanStagingBufferCache> stagingBufferCache;
        Scope<VulkanDescriptorCache> descriptorCache;
        Scope<VulkanPipelineCache> pipelineCache;
        Scope<VulkanSamplerCache> samplerCache;
        Scope<VulkanLayoutCache> layoutCache;
        Scope<Disposer> disposer;
        mutable FixedPool<VulkanBindHandle, 4096> bindhandlePool;
        mutable FixedPool<VulkanImageView, 2048> imageViewPool;
        mutable FixedPool<VulkanRawImage, 2048> imagePool;
        mutable FixedPool<VulkanRawBuffer, 2048> bufferPool;

        BuiltInResources* builtInResources;
        PropertyBlock globalResources = PropertyBlock(16384ull, 128ull);
    };
}