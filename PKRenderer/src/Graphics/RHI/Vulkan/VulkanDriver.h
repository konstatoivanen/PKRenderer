#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/Disposer.h"
#include "Graphics/RHI/RHIDriver.h"
#include "Graphics/RHI/Vulkan/VulkanCommon.h"
#include "Graphics/RHI/Vulkan/Services/VulkanDescriptorCache.h"
#include "Graphics/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Graphics/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Graphics/RHI/Vulkan/Services/VulkanStagingBufferCache.h"
#include "Graphics/RHI/Vulkan/Services/VulkanCommandBufferPool.h"
#include "Graphics/RHI/Vulkan/Services/VulkanFrameBufferCache.h"
#include "Graphics/RHI/Vulkan/Services/VulkanLayoutCache.h"
#include "Graphics/RHI/Vulkan/Services/VulkanBarrierHandler.h"
#include "Graphics/RHI/Vulkan/Objects/VulkanQueue.h"

namespace PK::Graphics::RHI::Vulkan
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

        APIType GetAPI() const final { return APIType::Vulkan; }
        RHIQueueSet* GetQueues() const final { return queues.get(); }
        std::string GetDriverHeader() const;
        DriverMemoryInfo GetMemoryInfo() const final;
        size_t GetBufferOffsetAlignment(BufferUsage usage) const final;
        BuiltInResources* GetBuiltInResources() final { return builtInResources; }
        Utilities::PropertyBlock* GetResourceState() final { return &globalResources; }

        RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) final;
        RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) final;
        RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) final;
        RHITextureBindArrayRef CreateTextureBindArray(size_t capacity) final;
        RHIBufferBindArrayRef CreateBufferBindArray(size_t capacity) final;
        RHIShaderScope CreateShader(void* base, PK::Assets::Shader::PKShaderVariant* pVariant, const char* name) final;
        RHIWindowScope CreateWindowScope(const WindowDescriptor& descriptor) final;

        void SetBuffer(Utilities::NameID name, RHIBuffer* buffer, const IndexRange& range) final;
        void SetTexture(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range) final;
        void SetBufferArray(Utilities::NameID name, RHIBindArray<RHIBuffer>* bufferArray) final;
        void SetTextureArray(Utilities::NameID name, RHIBindArray<RHITexture>* textureArray) final;
        void SetImage(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range) final;
        void SetSampler(Utilities::NameID name, const SamplerDescriptor& sampler) final;
        void SetAccelerationStructure(Utilities::NameID name, RHIAccelerationStructure* structure) final;
        void SetConstant(Utilities::NameID name, const void* data, uint32_t size) final;
        void SetKeyword(Utilities::NameID name, bool value) final;

        void WaitForIdle() const final { vkDeviceWaitIdle(device); }
        void GC() final;

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);

        void DisposePooledImageView(VulkanImageView* view, const Utilities::FenceRef& fence) const;
        void DisposePooledImage(VulkanRawImage* image, const Utilities::FenceRef& fence) const;
        void DisposePooledBuffer(VulkanRawBuffer* buffer, const Utilities::FenceRef& fence) const;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VmaAllocator allocator;
        VulkanContextProperties properties;
        VulkanPhysicalDeviceProperties physicalDeviceProperties;
        uint32_t apiVersion;

        Utilities::Scope<Objects::VulkanQueueSet> queues;
        Utilities::Scope<Services::VulkanFrameBufferCache> frameBufferCache;
        Utilities::Scope<Services::VulkanStagingBufferCache> stagingBufferCache;
        Utilities::Scope<Services::VulkanDescriptorCache> descriptorCache;
        Utilities::Scope<Services::VulkanPipelineCache> pipelineCache;
        Utilities::Scope<Services::VulkanSamplerCache> samplerCache;
        Utilities::Scope<Services::VulkanLayoutCache> layoutCache;
        Utilities::Scope<Utilities::Disposer> disposer;
        mutable Utilities::FixedPool<VulkanBindHandle, 4096> bindhandlePool;
        mutable Utilities::FixedPool<VulkanImageView, 2048> imageViewPool;
        mutable Utilities::FixedPool<VulkanRawImage, 2048> imagePool;
        mutable Utilities::FixedPool<VulkanRawBuffer, 2048> bufferPool;

        BuiltInResources* builtInResources;
        Utilities::PropertyBlock globalResources = Utilities::PropertyBlock(16384ull, 128ull);
    };
}