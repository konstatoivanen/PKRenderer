#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedUnique.h"
#include "Core/Utilities/Disposer.h"
#include "Core/Utilities/FixedTypeSet.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/Services/VulkanDescriptorCache.h"
#include "Core/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Core/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Core/RHI/Vulkan/Services/VulkanStagingBufferCache.h"
#include "Core/RHI/Vulkan/Services/VulkanCommandBufferPool.h"
#include "Core/RHI/Vulkan/Services/VulkanLayoutCache.h"
#include "Core/RHI/Vulkan/Services/VulkanBarrierHandler.h"
#include "Core/RHI/Vulkan/VulkanQueue.h"

namespace PK
{
    struct VulkanDriverDescriptor : public RHIDriverDescriptor
    {
        FixedString64 appName;
        FixedString64 engineName;
        FixedString256 workingDirectory;
        VulkanPhysicalDeviceFeatures features;
        const std::vector<const char*>* contextualInstanceExtensions;
        const std::vector<const char*>* contextualDeviceExtensions;

        VulkanDriverDescriptor(
            const char* appName = "Vulkan App",
            const char* engineName = "Vulkan Engine",
            const char* workingDirectory = "",
            RHIDriverDescriptor descriptor = {},
            VulkanPhysicalDeviceFeatures features = {},
            const std::vector<const char*>* contextualInstanceExtensions = nullptr,
            const std::vector<const char*>* contextualDeviceExtensions = nullptr) : RHIDriverDescriptor(descriptor),
            appName(appName),
            engineName(engineName),
            workingDirectory(workingDirectory),
            features(features),
            contextualInstanceExtensions(contextualInstanceExtensions),
            contextualDeviceExtensions(contextualDeviceExtensions)
        {
        }
    };

    struct VulkanDriver : public RHIDriver
    {
        VulkanDriver(const VulkanDriverDescriptor& descriptor);
        ~VulkanDriver();

        RHIAPI GetAPI() const final { return RHIAPI::Vulkan; }
        RHIQueueSet* GetQueues() const final { return queues.get(); }
        FixedString32 GetDriverHeader() const final;
        RHIDriverMemoryInfo GetMemoryInfo() const final;
        size_t GetBufferOffsetAlignment(BufferUsage usage) const final;
        BuiltInResources* GetBuiltInResources() final { return builtInResources; }
        PropertyBlock* GetResourceState() final { return &globalResources; }

        RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) final;
        RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) final;
        RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) final;
        RHITextureBindArrayRef CreateTextureBindArray(size_t capacity) final;
        RHIBufferBindArrayRef CreateBufferBindArray(size_t capacity) final;
        RHIShaderScope CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) final;
        RHISwapchainScope CreateSwapchain(const SwapchainDescriptor& descriptor) final;

        virtual RHIBuffer* AcquireStage(size_t size) final;
        virtual void ReleaseStage(RHIBuffer* buffer, const FenceRef& fence) final;

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

        template<typename T, typename ... Args>
        T* CreatePooled(Args&& ... args) const
        {
            auto pool = objectPools.GetInstance<IPool<T>>();
            return pool->New(std::forward<Args>(args)...);
        }

        template<typename T>
        void DisposePooled(T* object, const FenceRef& fence) const
        {
            auto deleter = [](void* v)
            {
                auto driver = RHIDriver::Get()->GetNative<VulkanDriver>();
                auto pool = driver->objectPools.GetInstance<IPool<T>>();
                pool->Delete(reinterpret_cast<T*>(v));
            };

            disposer->Dispose(object, deleter, fence);
        }

        template<typename T>
        void DeletePooled(T* object) const
        {
            objectPools.GetInstance<IPool<T>>()->Delete(object);
        }

        void* vkHandle;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VmaAllocator allocator;
        VulkanDriverDescriptor properties;
        VulkanPhysicalDeviceProperties physicalDeviceProperties;
        uint32_t apiVersion;

        // @TODO implement "optional" or something the like to inline this memory.
        mutable FixedUnique<VulkanQueueSet> queues;
        mutable FixedUnique<VulkanStagingBufferCache> stagingBufferCache;
        mutable FixedUnique<VulkanDescriptorCache> descriptorCache;
        mutable FixedUnique<VulkanPipelineCache> pipelineCache;
        mutable FixedUnique<VulkanSamplerCache> samplerCache;
        mutable FixedUnique<VulkanLayoutCache> layoutCache;
        mutable FixedUnique<Disposer> disposer;

        FixedUnique<BuiltInResources> builtInResources;
        PropertyBlock globalResources = PropertyBlock(16384ull, 128ull);

        mutable FixedTypeSet<
            FixedPool<VulkanBufferView, 2048>,
            FixedPool<VulkanImageView, 2048>,
            FixedPool<VulkanRawImage, 2048>,
            FixedPool<VulkanRawBuffer, 2048>,
            FixedPool<VulkanRawAccelerationStructure, 1024>> objectPools;
    };
}
