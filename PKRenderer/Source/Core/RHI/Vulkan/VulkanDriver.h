#pragma once
#include "Core/Base/Containers/TypeSet.h"
#include "Core/Base/Types/Ref.h"
#include "Core/Base/NoCopy.h"
#include "Core/ControlFlow/Disposer.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Core/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Core/RHI/Vulkan/Services/VulkanLayoutCache.h"
#include "Core/RHI/Vulkan/VulkanGraph.h"
#include "Core/RHI/Vulkan/VulkanAccessRecorder.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
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
        RHIGraph* GetGraph() const final { return graph.get(); }
        FixedString32 GetDriverHeader() const final;
        RHIDriverMemoryInfo GetMemoryInfo() const final;
        size_t GetBufferAlignment() const final;
        BuiltInResources* GetBuiltInResources() final { return builtInResources; }

        RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) final;
        RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) final;
        RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) final;
        RHITextureBindSetRef CreateTextureBindSet(size_t capacity) final;
        RHIBufferBindSetRef CreateBufferBindSet(size_t capacity) final;
        RHIShaderRef CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) final;
        RHISwapchainScope CreateSwapchain(const SwapchainDescriptor& descriptor) final;

        void WaitForIdle() const final { vkDeviceWaitIdle(device); }
        void GC() final;

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);

        void* vkHandle;
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VmaAllocator allocator = VK_NULL_HANDLE;
        VulkanDriverDescriptor properties;
        VulkanPhysicalDeviceProperties physicalDeviceProperties;
        uint32_t apiVersion;

        mutable FixedUnique<Disposer> disposer;
        mutable FixedUnique<VulkanPipelineCache> pipelineCache;
        mutable FixedUnique<VulkanSamplerCache> samplerCache;
        mutable FixedUnique<VulkanLayoutCache> layoutCache;
        mutable FixedUnique<VulkanQueueSet> queues;
        mutable FixedUnique<VulkanStagingArena> stagingArena;
        mutable FixedUnique<VulkanDescriptorArena> descriptorArena;
        mutable FixedUnique<VulkanTimerArena> timerArena;
        mutable FixedUnique<VulkanGraph> graph;
        mutable FixedUnique<BuiltInResources> builtInResources;

        FixedRefPool<VulkanTexture, PK_VK_MAX_IMAGES> texturePool;
        FixedRefPool<VulkanShader, PK_VK_MAX_SHADERS> shaderPool;
        FixedRefPool<VulkanBuffer, PK_VK_MAX_BUFFERS> bufferPool;
        mutable FixedPool<VulkanImageView, PK_VK_MAX_IMAGE_VIEWS> imageViewPool;
    };
}
