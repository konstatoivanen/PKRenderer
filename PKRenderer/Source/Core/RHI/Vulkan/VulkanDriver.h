#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedUnique.h"
#include "Core/Utilities/Disposer.h"
#include "Core/Utilities/FixedTypeSet.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
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
        ConstBufferView<const char*> instanceExtensions;
        ConstBufferView<const char*> deviceExtensions;

        VulkanDriverDescriptor(
            const char* appName = "Vulkan App",
            const char* engineName = "Vulkan Engine",
            const char* workingDirectory = "",
            RHIDriverDescriptor descriptor = {},
            VulkanPhysicalDeviceFeatures features = {},
            ConstBufferView<const char*> instanceExtensions = { nullptr, 0u },
            ConstBufferView<const char*> deviceExtensions = { nullptr, 0u }) : RHIDriverDescriptor(descriptor),
            appName(appName),
            engineName(engineName),
            workingDirectory(workingDirectory),
            features(features),
            instanceExtensions(instanceExtensions),
            deviceExtensions(deviceExtensions)
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
            disposer->Dispose(objectPools.GetInstance<IPool<T>>(), object, [](void* c, void* v)
            {
                reinterpret_cast<IPool<T>*>(c)->Delete(reinterpret_cast<T*>(v));
            }, 
            fence);
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

        mutable FixedUnique<VulkanQueueSet> queues;
        mutable FixedUnique<VulkanStagingBufferCache> stagingBufferCache;
        mutable FixedUnique<VulkanDescriptorCache> descriptorCache;
        mutable FixedUnique<VulkanPipelineCache> pipelineCache;
        mutable FixedUnique<VulkanSamplerCache> samplerCache;
        mutable FixedUnique<VulkanLayoutCache> layoutCache;
        mutable FixedUnique<Disposer> disposer;
        mutable FixedArena<PK_VK_FRAME_ARENA_SIZE> arena;
        
        FixedUnique<BuiltInResources> builtInResources;
        
        PropertyBlock globalResources = PropertyBlock(PK_VK_GLOBAL_PROPERTIES_INITIAL_SIZE, PK_VK_GLOBAL_PROPERTIES_INITIAL_COUNT);

        mutable FixedTypeSet<
            FixedPool<VulkanBufferView, PK_VK_MAX_BUFFER_VIEWS>,
            FixedPool<VulkanImageView, PK_VK_MAX_IMAGE_VIEWS>,
            FixedPool<VulkanRawImage, PK_VK_MAX_RAW_IMAGES>,
            FixedPool<VulkanRawBuffer, PK_VK_MAX_RAW_BUFFERS>,
            FixedPool<VulkanRawAccelerationStructure, PK_VK_MAX_ACCELERATION_STRUCTURES>> objectPools;
    };
}
