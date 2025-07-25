#pragma once
#include "Core/Platform/Platform.h"
#include "vulkan/vulkan.h"
#include "VMA/vk_mem_alloc.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Utilities/VersionedObject.h"
#include "Core/Utilities/FastLinkedList.h"
#include "Core/RHI/Structs.h"

#if PK_PLATFORM_WINDOWS
    #define PK_VK_LIBRARY_NAME "vulkan-1.dll"
#elif PK_PLATFORM_LINUX
    #define PK_VK_LIBRARY_NAME "libvulkan.so"
#elif
    #error "Unsupported platform!"
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#define PK_VK_SURFACE_EXTENSION_NAME VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_FUCHSIA)
#define PK_VK_SURFACE_EXTENSION_NAME VK_FUCHSIA_IMAGEPIPE_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_IOS_MVK)
#define PK_VK_SURFACE_EXTENSION_NAME VK_MVK_IOS_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#define PK_VK_SURFACE_EXTENSION_NAME VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#define PK_VK_SURFACE_EXTENSION_NAME VK_EXT_METAL_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_VI_NN)
#define PK_VK_SURFACE_EXTENSION_NAME VK_NN_VI_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#define PK_VK_SURFACE_EXTENSION_NAME VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#define PK_VK_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#define PK_VK_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#define PK_VK_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
#define PK_VK_SURFACE_EXTENSION_NAME VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
#define PK_VK_SURFACE_EXTENSION_NAME VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_GGP)
#define PK_VK_SURFACE_EXTENSION_NAME VK_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
#define PK_VK_SURFACE_EXTENSION_NAME VK_QNX_SCREEN_SURFACE_EXTENSION_NAME
#endif

extern PFN_vkSetDebugUtilsObjectNameEXT pkfn_vkSetDebugUtilsObjectNameEXT;
#define vkSetDebugUtilsObjectNameEXT pkfn_vkSetDebugUtilsObjectNameEXT

extern PFN_vkSetDebugUtilsObjectTagEXT pkfn_vkSetDebugUtilsObjectTagEXT;
#define vkSetDebugUtilsObjectTagEXT pkfn_vkSetDebugUtilsObjectTagEXT

extern PFN_vkQueueBeginDebugUtilsLabelEXT pkfn_vkQueueBeginDebugUtilsLabelEXT;
#define vkQueueBeginDebugUtilsLabelEXT pkfn_vkQueueBeginDebugUtilsLabelEXT

extern PFN_vkQueueEndDebugUtilsLabelEXT pkfn_vkQueueEndDebugUtilsLabelEXT;
#define vkQueueEndDebugUtilsLabelEXT pkfn_vkQueueEndDebugUtilsLabelEXT

extern PFN_vkQueueInsertDebugUtilsLabelEXT pkfn_vkQueueInsertDebugUtilsLabelEXT;
#define vkQueueInsertDebugUtilsLabelEXT pkfn_vkQueueInsertDebugUtilsLabelEXT

extern PFN_vkCmdBeginDebugUtilsLabelEXT pkfn_vkCmdBeginDebugUtilsLabelEXT;
#define vkCmdBeginDebugUtilsLabelEXT pkfn_vkCmdBeginDebugUtilsLabelEXT

extern PFN_vkCmdEndDebugUtilsLabelEXT pkfn_vkCmdEndDebugUtilsLabelEXT;
#define vkCmdEndDebugUtilsLabelEXT pkfn_vkCmdEndDebugUtilsLabelEXT

extern PFN_vkCmdInsertDebugUtilsLabelEXT pkfn_vkCmdInsertDebugUtilsLabelEXT;
#define vkCmdInsertDebugUtilsLabelEXT pkfn_vkCmdInsertDebugUtilsLabelEXT

extern PFN_vkCreateDebugUtilsMessengerEXT pkfn_vkCreateDebugUtilsMessengerEXT;
#define vkCreateDebugUtilsMessengerEXT pkfn_vkCreateDebugUtilsMessengerEXT

extern PFN_vkDestroyDebugUtilsMessengerEXT pkfn_vkDestroyDebugUtilsMessengerEXT;
#define vkDestroyDebugUtilsMessengerEXT pkfn_vkDestroyDebugUtilsMessengerEXT

extern PFN_vkSubmitDebugUtilsMessageEXT pkfn_vkSubmitDebugUtilsMessageEXT;
#define vkSubmitDebugUtilsMessageEXT pkfn_vkSubmitDebugUtilsMessageEXT

extern PFN_vkCreateAccelerationStructureKHR pkfn_vkCreateAccelerationStructureKHR;
#define vkCreateAccelerationStructureKHR pkfn_vkCreateAccelerationStructureKHR

extern PFN_vkDestroyAccelerationStructureKHR pkfn_vkDestroyAccelerationStructureKHR;
#define vkDestroyAccelerationStructureKHR pkfn_vkDestroyAccelerationStructureKHR

extern PFN_vkCmdSetRayTracingPipelineStackSizeKHR pkfn_vkCmdSetRayTracingPipelineStackSizeKHR;
#define vkCmdSetRayTracingPipelineStackSizeKHR pkfn_vkCmdSetRayTracingPipelineStackSizeKHR

extern PFN_vkCmdTraceRaysIndirectKHR pkfn_vkCmdTraceRaysIndirectKHR;
#define vkCmdTraceRaysIndirectKHR pkfn_vkCmdTraceRaysIndirectKHR

extern PFN_vkCmdTraceRaysKHR pkfn_vkCmdTraceRaysKHR;
#define vkCmdTraceRaysKHR pkfn_vkCmdTraceRaysKHR

extern PFN_vkCreateRayTracingPipelinesKHR pkfn_vkCreateRayTracingPipelinesKHR;
#define vkCreateRayTracingPipelinesKHR pkfn_vkCreateRayTracingPipelinesKHR

extern PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pkfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR;
#define vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pkfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR

extern PFN_vkGetRayTracingShaderGroupHandlesKHR pkfn_vkGetRayTracingShaderGroupHandlesKHR;
#define vkGetRayTracingShaderGroupHandlesKHR pkfn_vkGetRayTracingShaderGroupHandlesKHR

extern PFN_vkGetRayTracingShaderGroupStackSizeKHR pkfn_vkGetRayTracingShaderGroupStackSizeKHR;
#define vkGetRayTracingShaderGroupStackSizeKHR pkfn_vkGetRayTracingShaderGroupStackSizeKHR

extern PFN_vkGetAccelerationStructureDeviceAddressKHR pkfn_vkGetAccelerationStructureDeviceAddressKHR;
#define vkGetAccelerationStructureDeviceAddressKHR pkfn_vkGetAccelerationStructureDeviceAddressKHR

extern PFN_vkGetAccelerationStructureBuildSizesKHR pkfn_vkGetAccelerationStructureBuildSizesKHR;
#define vkGetAccelerationStructureBuildSizesKHR pkfn_vkGetAccelerationStructureBuildSizesKHR

extern PFN_vkCmdBuildAccelerationStructuresKHR pkfn_vkCmdBuildAccelerationStructuresKHR;
#define vkCmdBuildAccelerationStructuresKHR pkfn_vkCmdBuildAccelerationStructuresKHR

extern PFN_vkCmdCopyAccelerationStructureKHR pkfn_vkCmdCopyAccelerationStructureKHR;
#define vkCmdCopyAccelerationStructureKHR pkfn_vkCmdCopyAccelerationStructureKHR

extern PFN_vkCmdPipelineBarrier2KHR pkfn_vkCmdPipelineBarrier2KHR;
#define vkCmdPipelineBarrier2KHR pkfn_vkCmdPipelineBarrier2KHR

extern PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR;
#define vkCmdWriteAccelerationStructuresPropertiesKHR pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR

extern PFN_vkCmdDrawMeshTasksEXT pkfn_vkCmdDrawMeshTasksEXT;
#define vkCmdDrawMeshTasksEXT pkfn_vkCmdDrawMeshTasksEXT

extern PFN_vkCmdDrawMeshTasksIndirectEXT pkfn_vkCmdDrawMeshTasksIndirectEXT;
#define vkCmdDrawMeshTasksIndirectEXT pkfn_vkCmdDrawMeshTasksIndirectEXT

extern PFN_vkCmdDrawMeshTasksIndirectCountEXT pkfn_vkCmdDrawMeshTasksIndirectCountEXT;
#define vkCmdDrawMeshTasksIndirectCountEXT pkfn_vkCmdDrawMeshTasksIndirectCountEXT

extern PFN_vkAcquireFullScreenExclusiveModeEXT pkfn_vkAcquireFullScreenExclusiveModeEXT;
#define vkAcquireFullScreenExclusiveModeEXT pkfn_vkAcquireFullScreenExclusiveModeEXT

extern PFN_vkReleaseFullScreenExclusiveModeEXT pkfn_vkReleaseFullScreenExclusiveModeEXT;
#define vkReleaseFullScreenExclusiveModeEXT pkfn_vkReleaseFullScreenExclusiveModeEXT

namespace PK
{
    struct VulkanQueueFamilies
    {
        uint32_t indices[(uint32_t)QueueType::MaxCount]{};
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

    struct VulkanPhysicalDeviceProperties
    {
        VkPhysicalDeviceProperties core;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracing;
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructure;
        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterization;
        VkPhysicalDeviceSubgroupProperties subgroup;
        VkPhysicalDeviceMeshShaderPropertiesEXT meshShader;
    };

    struct VulkanPhysicalDeviceFeatures
    {
        VkPhysicalDeviceFeatures2 vk10{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan11Features vk11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features vk12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceVulkan13Features vk13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        VkPhysicalDeviceVulkan14Features vk14{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
        VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloat{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT };
        VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR positionFetch{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR };
        VkPhysicalDeviceMeshShaderFeaturesEXT meshshader{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
        VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRate{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
        VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT fifoLatestReady{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_EXT };
        VulkanPhysicalDeviceFeatures();
        static bool CheckRequirements(const VulkanPhysicalDeviceFeatures& requirements, const VulkanPhysicalDeviceFeatures available);
    };

    struct VulkanPhysicalDeviceRequirements
    {
        uint32_t versionMajor;
        uint32_t versionMinor;
        VkPhysicalDeviceType deviceType;
        VulkanPhysicalDeviceFeatures features;
        const std::vector<const char*>* deviceExtensions;
    };

    struct VulkanExclusiveFullscreenInfo
    {
#if PK_PLATFORM_WINDOWS
        VkSurfaceFullScreenExclusiveWin32InfoEXT win32Info;
        VkSurfaceFullScreenExclusiveInfoEXT fullscreenInfo;
#endif
        void* swapchainPNext = nullptr;
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
        VulkanBufferCreateInfo(BufferUsage usage, size_t size, const VulkanQueueFamilies* families = nullptr);

        VkBufferCreateInfo buffer { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        VmaAllocationCreateInfo allocation {};
        VulkanQueueFamilies queueFamilies{};
    };

    struct VulkanImageCreateInfo
    {
        VulkanImageCreateInfo() {};
        VulkanImageCreateInfo(const TextureDescriptor& descriptor, const VulkanQueueFamilies* families = nullptr);

        VkImageCreateInfo image = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        VmaAllocationCreateInfo allocation = {};
        VkFormat formatAlias = VK_FORMAT_MAX_ENUM;
        VulkanQueueFamilies queueFamilies{};
    };

    struct VulkanImageViewCreateInfo
    {
        VkImage image;
        VkImage imageAlias;
        VkImageViewType  viewType;
        VkFormat format;
        VkFormat formatAlias;
        VkImageLayout layout;
        VkSampleCountFlagBits samples;
        VkComponentMapping components;
        VkExtent3D extent;
        VkImageSubresourceRange subresourceRange;
        bool isConcurrent;
        bool isTracked;
        bool isAlias;
    };

    struct VulkanBindHandle : VersionedObject
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
                uint16_t samples = (uint16_t)VK_SAMPLE_COUNT_1_BIT; // VkSampleCountFlagBits
            } 
            image;

            struct Buffer
            {
                VkBuffer buffer;
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

    struct VulkanBufferView : public FastLinkedListElement<VulkanBufferView, BufferIndexRange>, public VulkanBindHandle
    {
    };

    struct VulkanImageView : public VersionedObject, public FastLinkedListElement<VulkanImageView, uint64_t>
    {
        VulkanImageView(VkDevice device, const VulkanImageViewCreateInfo& createInfo, const char* name);
        ~VulkanImageView();

        const VkDevice device;
        VkImageView view;
        VulkanBindHandle bindHandle;
    };

    struct VulkanRawBuffer : public VersionedObject
    {
        VulkanRawBuffer(VkDevice device, VmaAllocator allocator, const VulkanBufferCreateInfo& createInfo, const char* name);
        ~VulkanRawBuffer();

        void* BeginMap(size_t offset, size_t readsize) const;
        void EndMap(size_t offset, size_t size) const;

        const VmaAllocator allocator;
        const VkDeviceSize size;
        const bool isPersistentMap;
        VmaAllocation memory;
        VkBuffer buffer;
        VkDeviceAddress deviceAddress;
    };

    struct VulkanRawImage : public VersionedObject
    {
        VulkanRawImage(VkDevice device, VmaAllocator allocator, const VulkanImageCreateInfo& createInfo, const char* name);
        ~VulkanRawImage();

        const VmaAllocator allocator;
        VmaAllocation memory;
        VkImage image;
        VkImage imageAlias;
        VkSampleCountFlagBits samples;
        VkFormat format;
        VkFormat formatAlias;
        VkImageType type;
        VkExtent3D extent;
        uint32_t levels;
        uint32_t layers;
    };

    struct VulkanRawAccelerationStructure : public VersionedObject
    {
        VulkanRawAccelerationStructure(VkDevice device, const VkAccelerationStructureCreateInfoKHR& createInfo, const char* name);
        ~VulkanRawAccelerationStructure();
        const VkDevice device;
        VkAccelerationStructureKHR structure;
        VkDeviceAddress deviceAddress;
    };

    struct VulkanPipeline : public NoCopy
    {
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo, const char* name);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo, const char* name);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkRayTracingPipelineCreateInfoKHR& createInfo, const char* name);
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
        VulkanDescriptorSetLayout(VkDevice device, 
                                  const VkDescriptorSetLayoutCreateInfo& createInfo, 
                                  VkShaderStageFlagBits stageFlags, 
                                  const char* name);
        ~VulkanDescriptorSetLayout();

        const VkDevice device;
        VkDescriptorSetLayout layout;
        VkShaderStageFlagBits stageFlags;
        FixedString128 name;
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
        mutable FenceRef fence;
    };

    struct VulkanSampler : public NoCopy
    {
        VulkanSampler(VkDevice device, const SamplerDescriptor& descriptor, const char* name);
        ~VulkanSampler();

        const VkDevice device;
        VkSampler sampler;
    };

    struct VulkanQueryPool : public NoCopy
    {
        VulkanQueryPool(VkDevice device, VkQueryType type, uint32_t size);
        ~VulkanQueryPool();

        bool WaitResults(uint64_t timeout);
        void ResetQuery();
        int32_t AddQuery(const FenceRef& fence);

        bool GetResults(void* outBuffer, size_t first, size_t count, size_t stride, uint64_t timeout, VkQueryResultFlagBits flags);
        bool GetResultsAll(void* outBuffer, size_t stride, uint64_t timeout, VkQueryResultFlagBits flags);

        template<typename T>
        bool GetResults(T* outBuffer, size_t first, size_t count, uint64_t timeout, VkQueryResultFlagBits flags)
        {
            return GetResults(outBuffer, first, count, sizeof(T), timeout, flags);
        }

        template<typename T>
        T GetResult(size_t index, uint64_t timeout, VkQueryResultFlagBits flags)
        {
            T outValue = T();
            GetResults(&outValue, index, 1u, sizeof(T), timeout, flags);
            return outValue;
        }

        template<typename T>
        bool GetResultsAll(T* outBuffer, uint64_t timeout, VkQueryResultFlagBits flags)
        {
            return GetResultsAll(outBuffer, sizeof(T), timeout, flags);
        }

        const VkDevice device;
        const uint32_t size;
        const VkQueryType type;
        FenceRef lastQueryFence;
        uint32_t count;
        VkQueryPool pool;
    };

    namespace VulkanEnumConvert
    {
        VkFormat GetFormat(ElementType format);
        ElementType GetUniformFormat(VkFormat format);
        VkColorSpaceKHR GetColorSpace(ColorSpace colorSpace);
        VkPresentModeKHR GetPresentMode(VSyncMode vsyncMode);
        ColorSpace GetColorSpace(VkColorSpaceKHR colorSpace);
        VSyncMode GetVSyncMode(VkPresentModeKHR presentMode);
        VkFormat GetFormat(TextureFormat format);
        VkIndexType GetIndexType(ElementType format);
        TextureFormat GetTextureFormat(VkFormat format);
        uint32_t GetFormatBlockSize(VkFormat format);
        bool IsDepthFormat(VkFormat format);
        bool IsDepthStencilFormat(VkFormat format);

        VkClearValue GetClearValue(const TextureClearValue& clearValue);
        VkComponentMapping GetSwizzle(VkFormat format);
        VkImageViewType GetViewType(TextureType type);
        VkImageLayout GetImageLayout(TextureUsage usage);
        VkAttachmentLoadOp GetLoadOp(LoadOp loadOp);
        VkAttachmentStoreOp GetStoreOp(StoreOp storeOp);
        VkCompareOp GetCompareOp(Comparison comparison);
        VkBorderColor GetBorderColor(BorderColor color);
        VkSamplerAddressMode GetSamplerAddressMode(WrapMode wrap);
        VkFilter GetFilterMode(FilterMode filter);
        VkDescriptorType GetDescriptorType(ShaderResourceType type);
        ShaderResourceType GetShaderResourceType(VkDescriptorType type);
        VkShaderStageFlagBits GetShaderStage(ShaderStage stage);
        VkPipelineBindPoint GetPipelineBindPoint(ShaderStageFlags stageFlags);
        VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples);
        VkVertexInputRate GetInputRate(InputRate inputRate);
        VkShaderStageFlagBits GetShaderStageFlags(ShaderStageFlags stageFlags);
        VkPolygonMode GetPolygonMode(PolygonMode mode);
        VkBlendOp GetBlendOp(BlendOp op);
        VkBlendFactor GetBlendFactor(BlendFactor factor, VkBlendFactor fallback);
        VkLogicOp GetLogicOp(LogicOp op);
        VkCullModeFlagBits GetCullMode(CullMode op);
        VkConservativeRasterizationModeEXT GetRasterMode(RasterMode mode);
        VkPrimitiveTopology GetTopology(Topology topology);
        VkFrontFace GetFrontFace(FrontFace face);
        VkPipelineStageFlags GetPipelineStageFlags(VkShaderStageFlags flags);
        VkRayTracingShaderGroupTypeKHR GetRayTracingStageGroupType(ShaderStage stage);
        VkImageAspectFlagBits GetFormatAspect(VkFormat format);
        uint32_t ExpandVkRange16(uint32_t v);
        bool IsReadAccess(VkAccessFlags flags);
        bool IsWriteAccess(VkAccessFlags flags);
    }

    VkResult VulkanCreateSurfaceKHR(VkInstance instance, void* nativeWindow, VkSurfaceKHR* surface);

    void VulkanBindExtensionMethods(VkInstance instance, bool enableDebugNames);

    std::vector<VkPhysicalDevice> VulkanGetPhysicalDevices(VkInstance instance);
    std::vector<VkQueueFamilyProperties> VulkanGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device);
    VulkanPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device);
    VulkanExclusiveFullscreenInfo VulkanGetSwapchainFullscreenInfo(const void* nativeMonitor, bool fullScreen);

    bool VulkanValidateInstanceExtensions(const std::vector<const char*>* extensions);
    bool VulkanValidatePhysicalDeviceExtensions(VkPhysicalDevice device, const std::vector<const char*>* extensions);
    bool VulkanValidateValidationLayers(const char* const* validationLayers, const uint32_t count);
    bool VulkanIsPresentSupported(VkPhysicalDevice physicalDevice, uint32_t familyIndex, VkSurfaceKHR surface);

    void VulkanSelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* device);
    VkExtent2D VulkanSelectSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& desiredExtent);
    VkSurfaceFormatKHR VulkanSelectSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkFormat desiredFormat, VkColorSpaceKHR desiredColorSpace);
    VkPresentModeKHR VulkanSelectPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkPresentModeKHR desiredPresentMode);

    VkAccelerationStructureBuildSizesInfoKHR VulkanGetAccelerationBuildSizesInfo(VkDevice device, const VkAccelerationStructureBuildGeometryInfoKHR info, uint32_t primitiveCount);
    
    // Defined here to prevent multiple includes of vulkan.h with wrong defines.
    FixedString128 VulkanStr_VkQueueFlags(VkQueueFlags value);
    const char* VulkanCStr_VkShaderStageFlagBits(VkShaderStageFlagBits value);
    const char* VulkanCStr_VkFormat(VkFormat value);
    const char* VulkanCStr_VkColorSpaceKHR(VkColorSpaceKHR value);
    const char* VulkanCStr_VkPresentModeKHR(VkPresentModeKHR value);

    VkImageSubresourceRange VulkanConvertRange(const TextureViewRange& viewRange, VkImageAspectFlags aspect);
    TextureViewRange VulkanConvertRange(const VkImageSubresourceRange& resourceRange);

    void VulkanSetObjectDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name);

    void VulkanAssertAPIVersion(const uint32_t major, const uint32_t minor);

    void VulkanThrowError(VkResult result, const char* context);

    #define VK_ASSERT_RESULT_CTX(cmd, ctx) \
    { \
        auto result = cmd; \
        if (result != VK_SUCCESS) \
        { \
            VulkanThrowError(result, ctx); \
        } \
    } \

    #define VK_ASSERT_RESULT(cmd) VK_ASSERT_RESULT_CTX(cmd, "VK COMMAND FAILED! ")
    #define VK_THROW_RESULT(result) VulkanThrowError(result, "VK Error Result: ")
    #define VK_THROW_RESULT_CTX(result, ctx) VulkanThrowError(result, ctx)

}
