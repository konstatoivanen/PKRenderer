#pragma once
#include "Core/Platform/Platform.h"
#define VMA_SYSTEM_ALIGNED_MALLOC(size, alignment) PK::Memory::AllocateAligned(size, alignment)
#define VMA_SYSTEM_FREE(ptr) PK::Memory::Free(ptr)
#define VMA_STATS_STRING_ENABLED 0
#include "vulkan/vulkan.h"
#include "VMA/vk_mem_alloc.h"
#include "Core/Base/NoCopy.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/ControlFlow/FenceRef.h"
#include "Core/RHI/Structs.h"

constexpr static const char* VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";

#define VK_ASSERT_RESULT_CTX(cmd, ctx) \
    { \
        auto result = cmd; \
        if (result != VK_SUCCESS) \
        { \
            PK:: VulkanThrowError(result, ctx); \
        } \
    } \

#define VK_ASSERT_RESULT(cmd) VK_ASSERT_RESULT_CTX(cmd, "VK Error Result:")
#define VK_THROW_RESULT(result) PK::VulkanThrowError(result, "VK Error Result:")
#define VK_THROW_RESULT_CTX(result, ctx) PK::VulkanThrowError(result, ctx)

#define PK_VK_IMAGE_RANGE_MAX 0x7FFFu

#define PK_VK_ADDRESS_FLAGS (VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR)

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

extern PFN_vkCreateAccelerationStructure2KHR pkfn_vkCreateAccelerationStructure2KHR;
#define vkCreateAccelerationStructure2KHR pkfn_vkCreateAccelerationStructure2KHR

extern PFN_vkDestroyAccelerationStructureKHR pkfn_vkDestroyAccelerationStructureKHR;
#define vkDestroyAccelerationStructureKHR pkfn_vkDestroyAccelerationStructureKHR

extern PFN_vkCmdTraceRaysIndirectKHR pkfn_vkCmdTraceRaysIndirectKHR;
#define vkCmdTraceRaysIndirectKHR pkfn_vkCmdTraceRaysIndirectKHR

extern PFN_vkCmdTraceRaysKHR pkfn_vkCmdTraceRaysKHR;
#define vkCmdTraceRaysKHR pkfn_vkCmdTraceRaysKHR

extern PFN_vkCreateRayTracingPipelinesKHR pkfn_vkCreateRayTracingPipelinesKHR;
#define vkCreateRayTracingPipelinesKHR pkfn_vkCreateRayTracingPipelinesKHR

extern PFN_vkGetRayTracingShaderGroupHandlesKHR pkfn_vkGetRayTracingShaderGroupHandlesKHR;
#define vkGetRayTracingShaderGroupHandlesKHR pkfn_vkGetRayTracingShaderGroupHandlesKHR

extern PFN_vkGetAccelerationStructureDeviceAddressKHR pkfn_vkGetAccelerationStructureDeviceAddressKHR;
#define vkGetAccelerationStructureDeviceAddressKHR pkfn_vkGetAccelerationStructureDeviceAddressKHR

extern PFN_vkGetAccelerationStructureBuildSizesKHR pkfn_vkGetAccelerationStructureBuildSizesKHR;
#define vkGetAccelerationStructureBuildSizesKHR pkfn_vkGetAccelerationStructureBuildSizesKHR

extern PFN_vkCmdBuildAccelerationStructuresKHR pkfn_vkCmdBuildAccelerationStructuresKHR;
#define vkCmdBuildAccelerationStructuresKHR pkfn_vkCmdBuildAccelerationStructuresKHR

extern PFN_vkCmdCopyAccelerationStructureKHR pkfn_vkCmdCopyAccelerationStructureKHR;
#define vkCmdCopyAccelerationStructureKHR pkfn_vkCmdCopyAccelerationStructureKHR

extern PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR;
#define vkCmdWriteAccelerationStructuresPropertiesKHR pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR

extern PFN_vkCmdDrawMeshTasksEXT pkfn_vkCmdDrawMeshTasksEXT;
#define vkCmdDrawMeshTasksEXT pkfn_vkCmdDrawMeshTasksEXT

extern PFN_vkCmdDrawMeshTasksIndirect2EXT pkfn_vkCmdDrawMeshTasksIndirect2EXT;
#define vkCmdDrawMeshTasksIndirect2EXT pkfn_vkCmdDrawMeshTasksIndirect2EXT

extern PFN_vkCmdDrawMeshTasksIndirectCount2EXT pkfn_vkCmdDrawMeshTasksIndirectCount2EXT;
#define vkCmdDrawMeshTasksIndirectCount2EXT pkfn_vkCmdDrawMeshTasksIndirectCount2EXT

extern PFN_vkAcquireFullScreenExclusiveModeEXT pkfn_vkAcquireFullScreenExclusiveModeEXT;
#define vkAcquireFullScreenExclusiveModeEXT pkfn_vkAcquireFullScreenExclusiveModeEXT

extern PFN_vkReleaseFullScreenExclusiveModeEXT pkfn_vkReleaseFullScreenExclusiveModeEXT;
#define vkReleaseFullScreenExclusiveModeEXT pkfn_vkReleaseFullScreenExclusiveModeEXT

extern PFN_vkWaitForPresentKHR pkfn_vkWaitForPresentKHR;
#define vkWaitForPresentKHR pkfn_vkWaitForPresentKHR

extern PFN_vkCmdBindIndexBuffer3KHR pkfn_vkCmdBindIndexBuffer3KHR;
#define vkCmdBindIndexBuffer3KHR pkfn_vkCmdBindIndexBuffer3KHR

extern PFN_vkCmdBindVertexBuffers3KHR pkfn_vkCmdBindVertexBuffers3KHR;
#define vkCmdBindVertexBuffers3KHR pkfn_vkCmdBindVertexBuffers3KHR

extern PFN_vkCmdCopyImageToMemoryKHR pkfn_vkCmdCopyImageToMemoryKHR;
#define vkCmdCopyImageToMemoryKHR pkfn_vkCmdCopyImageToMemoryKHR

extern PFN_vkCmdCopyMemoryKHR pkfn_vkCmdCopyMemoryKHR;
#define vkCmdCopyMemoryKHR pkfn_vkCmdCopyMemoryKHR

extern PFN_vkCmdCopyMemoryToImageKHR pkfn_vkCmdCopyMemoryToImageKHR;
#define vkCmdCopyMemoryToImageKHR pkfn_vkCmdCopyMemoryToImageKHR

extern PFN_vkCmdDispatchIndirect2KHR pkfn_vkCmdDispatchIndirect2KHR;
#define vkCmdDispatchIndirect2KHR pkfn_vkCmdDispatchIndirect2KHR

extern PFN_vkCmdDrawIndexedIndirect2KHR pkfn_vkCmdDrawIndexedIndirect2KHR;
#define vkCmdDrawIndexedIndirect2KHR pkfn_vkCmdDrawIndexedIndirect2KHR

extern PFN_vkCmdDrawIndirect2KHR pkfn_vkCmdDrawIndirect2KHR;
#define vkCmdDrawIndirect2KHR pkfn_vkCmdDrawIndirect2KHR

extern PFN_vkCmdFillMemoryKHR pkfn_vkCmdFillMemoryKHR;
#define vkCmdFillMemoryKHR pkfn_vkCmdFillMemoryKHR

extern PFN_vkCmdUpdateMemoryKHR pkfn_vkCmdUpdateMemoryKHR;
#define vkCmdUpdateMemoryKHR pkfn_vkCmdUpdateMemoryKHR

extern PFN_vkWriteResourceDescriptorsEXT pkfn_vkWriteResourceDescriptorsEXT;
#define vkWriteResourceDescriptorsEXT pkfn_vkWriteResourceDescriptorsEXT

extern PFN_vkWriteSamplerDescriptorsEXT pkfn_vkWriteSamplerDescriptorsEXT;
#define vkWriteSamplerDescriptorsEXT pkfn_vkWriteSamplerDescriptorsEXT

extern PFN_vkCmdBindResourceHeapEXT pkfn_vkCmdBindResourceHeapEXT;
#define vkCmdBindResourceHeapEXT pkfn_vkCmdBindResourceHeapEXT

extern PFN_vkCmdBindSamplerHeapEXT pkfn_vkCmdBindSamplerHeapEXT;
#define vkCmdBindSamplerHeapEXT pkfn_vkCmdBindSamplerHeapEXT

extern PFN_vkCmdPushDataEXT pkfn_vkCmdPushDataEXT;
#define vkCmdPushDataEXT pkfn_vkCmdPushDataEXT

namespace PK
{
    struct VulkanQueueFamilies
    {
        uint32_t indices[(uint32_t)QueueType::EnumCount]{};
        uint32_t count;
    };

    struct VulkanDescriptorHeapProperties
    {
        VkDeviceSize imageStride = 0ull;
        VkDeviceSize bufferStride = 0ull;
        VkDeviceSize maxResourceStride = 0ull;
        VkDeviceSize samplerStride = 0ull;
        VkDeviceSize resourceHeapOffset = 0ull;
        VkDeviceSize samplerHeapOffset = 0ull;
        VkDeviceSize resourceHeapAlignment = 0ull;
        VkDeviceSize samplerHeapAlignment = 0ull;
        VkDeviceSize maxResourceHeapSize = 0ull;
        VkDeviceSize maxSamplerHeapSize = 0ull;
    };

    struct VulkanPhysicalDeviceProperties
    {
        VkPhysicalDeviceProperties core;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracing;
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructure;
        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterization;
        VkPhysicalDeviceSubgroupProperties subgroup;
        VkPhysicalDeviceMeshShaderPropertiesEXT meshShader;
        VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeap;
        VulkanDescriptorHeapProperties descriptorHeapDerived;
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
        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT };
        VkPhysicalDevicePresentIdFeaturesKHR presentId{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };
        VkPhysicalDevicePresentWaitFeaturesKHR presentWait{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR };
        VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR maximalReconvergence { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MAXIMAL_RECONVERGENCE_FEATURES_KHR };
        VkPhysicalDeviceShaderQuadControlFeaturesKHR quadControl{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_QUAD_CONTROL_FEATURES_KHR };
        VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptorHeap { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT };
        VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unifiedLayout { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR };
        VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR deviceAddressCommands { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_ADDRESS_COMMANDS_FEATURES_KHR };
        VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR raytracingMaintenance1 { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR };
        VulkanPhysicalDeviceFeatures();
        static bool CheckRequirements(const VulkanPhysicalDeviceFeatures& requirements, const VulkanPhysicalDeviceFeatures available);
    };

    struct VulkanPhysicalDeviceRequirements
    {
        uint32_t versionMajor;
        uint32_t versionMinor;
        VkPhysicalDeviceType deviceType;
        VulkanPhysicalDeviceFeatures features;
        const char* const* deviceExtensions;
        size_t deviceExtensionCount;
    };

    struct VulkanExclusiveFullscreenInfo
    {
#if PK_PLATFORM_WINDOWS
        VkSurfaceFullScreenExclusiveWin32InfoEXT win32Info;
        VkSurfaceFullScreenExclusiveInfoEXT fullscreenInfo;
#endif
        void* swapchainPNext = nullptr;
    };


    struct VulkanResourceView
    {
        uint64_t hash = 0ull;

        union
        {
            struct
            {
                VkImage image = VK_NULL_HANDLE;
                VkFormat format = VK_FORMAT_UNDEFINED;
                VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
                uint16_t samplerIndex = 0u;
                uint16_t samples = (uint16_t)VK_SAMPLE_COUNT_1_BIT;
                VkExtent3D extent = { 0u, 0u, 0u };
                VkImageSubresourceRange range = { VK_IMAGE_ASPECT_NONE, 0u, VK_REMAINING_MIP_LEVELS, 0u, VK_REMAINING_ARRAY_LAYERS };
            }
            image;

            VkDeviceAddressRangeKHR memory;
        };

        VulkanResourceView() : image{} {}

        constexpr bool operator == (const VulkanResourceView& r) const noexcept { return hash == r.hash; }
        constexpr bool operator != (const VulkanResourceView& r) const noexcept { return hash != r.hash; }

        inline void UpdateHashMemory()
        {
            hash = Hash::Mix(memory.address, memory.size);
        }

        inline void UpdateHashImage(uint64_t version = 0ull)
        {
            auto h2 = 0ull;
            h2 |= ((uint64_t)image.range.baseMipLevel) << 48ull;
            h2 |= ((uint64_t)(image.range.levelCount & 0xFFFFu)) << 32ull;
            h2 |= ((uint64_t)image.range.baseArrayLayer) << 16ull;
            h2 |= ((uint64_t)(image.range.layerCount & 0xFFFFu));
            const auto h0 = Hash::Mix((uint64_t)image.image, version ^ ((uint64_t)image.format << 16) ^ (uint64_t)image.viewType);
            const auto h1 = Hash::Mix((uint64_t)image.samplerIndex, h2);
            hash = Hash::Mix(h0, h1);
        }
    };

    struct VulkanDescriptorBinding
    {
        const VulkanResourceView* views;
        uint64_t hash;
        uint16_t count;
        ShaderResourceType type;
    };

    struct VulkanPipeline : public NoCopy
    {
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo& createInfo, const char* name);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkComputePipelineCreateInfo& createInfo, const char* name);
        VulkanPipeline(VkDevice device, VkPipelineCache pipelineCache, const VkRayTracingPipelineCreateInfoKHR& createInfo, const char* name);
        ~VulkanPipeline();

        const VkDevice device;
        VkPipeline pipeline;
        VkPipelineBindPoint bindPoint;
    };

    struct VulkanQueryPool : public NoCopy
    {
        VulkanQueryPool(VkDevice device, VkQueryType type, uint32_t size);
        ~VulkanQueryPool();

        void GetResults(void* outBuffer, uint32_t first, uint32_t count, size_t stride, VkQueryResultFlagBits flags);

        template<typename T>
        void GetResults(T* outBuffer, uint32_t first, uint32_t count, VkQueryResultFlagBits flags)
        {
            GetResults(outBuffer, first, count, sizeof(T), flags);
        }

        template<typename T>
        T GetResult(uint32_t index, VkQueryResultFlagBits flags)
        {
            T outValue = T();
            GetResults(&outValue, index, 1u, sizeof(T), flags);
            return outValue;
        }

        void ResetQuery(uint32_t first, uint32_t count);
        bool WaitResults(uint64_t timeout);
        void SetFence(const FenceRef& fence);

        const VkDevice device;
        const uint32_t size;
        const VkQueryType type;
        FenceRef fence;
        VkQueryPool pool;
    };

    struct VulkanDynamicState
    {
        DepthStencilParameters depthStencil{};
        const VkViewport* viewports = nullptr;
        const VkRect2D* scissors = nullptr;
        uint16_t viewportCount = 0u;
        uint16_t scissorCount = 0u;
        CullMode cullMode = CullMode::Off;
        FrontFace frontFace = FrontFace::CounterClockwise;
        bool rasterizerDiscardEnable = false;
        bool depthBiasEnable = false;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasClamp = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float lineWidth = 1.0f;
    };

    void VulkanBindExtensionMethods(VkInstance instance, bool enableDebugNames, bool enableDebugLabels);

    VkResult VulkanCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
    VkResult VulkanCreateSurfaceKHR(VkInstance instance, void* nativeWindow, VkSurfaceKHR* surface);

    VkAccelerationStructureBuildSizesInfoKHR VulkanGetAccelerationBuildSizesInfo(VkDevice device, const VkAccelerationStructureBuildGeometryInfoKHR info, uint32_t primitiveCount);
    VkDeviceAddress VulkanGetAccelerationStructureDeviceAddress(VkDevice device, VkAccelerationStructureKHR handle);
    VkDeviceAddress VulkanGetBudderDeviceAddress(VkDevice device, VkBuffer buffer);
    VulkanPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device);
    VulkanExclusiveFullscreenInfo VulkanGetSwapchainFullscreenInfo(const void* nativeMonitor, bool fullScreen);

    bool VulkanValidatePhysicalDeviceExtensions(VkPhysicalDevice device, const char* const* extensions, size_t count);
    bool VulkanIsPresentSupported(VkPhysicalDevice physicalDevice, uint32_t familyIndex, VkSurfaceKHR surface);

    void VulkanSelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* device);
    VkExtent2D VulkanSelectSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& desiredExtent);
    VkSurfaceFormatKHR VulkanSelectSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkFormat desiredFormat, VkColorSpaceKHR desiredColorSpace);
    uint32_t VulkanQueryPresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkPresentModeKHR* desiredMode, VkPresentModeKHR* outModes, uint32_t capacity);

    // Defined here to prevent multiple includes of vulkan.h with wrong defines.
    FixedString128 VulkanStr_VkQueueFlags(VkQueueFlags value);
    const char* VulkanCStr_VkShaderStageFlagBits(VkShaderStageFlagBits value);
    const char* VulkanCStr_VkFormat(VkFormat value);
    const char* VulkanCStr_VkColorSpaceKHR(VkColorSpaceKHR value);
    const char* VulkanCStr_VkPresentModeKHR(VkPresentModeKHR value);

    void VulkanSetObjectDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name);
    void VulkanAssertAPIVersion(const uint32_t major, const uint32_t minor);
    void VulkanThrowError(VkResult result, const char* context);

    namespace VulkanEnumConvert
    {
        VkFormat GetFormat(ElementType format);
        VkColorSpaceKHR GetColorSpace(ColorSpace colorSpace);
        VkPresentModeKHR GetPresentMode(VSyncMode vsyncMode);
        ColorSpace GetColorSpace(VkColorSpaceKHR colorSpace);
        VSyncMode GetVSyncMode(VkPresentModeKHR presentMode);
        VkFormat GetFormat(TextureFormat format);
        VkIndexType GetIndexType(size_t size);
        TextureFormat GetTextureFormat(VkFormat format);
        uint32_t GetFormatBlockSize(VkFormat format);
        bool IsBlockCompressedFormat(VkFormat format);
        bool IsDepthFormat(VkFormat format);
        bool IsDepthStencilFormat(VkFormat format);

        VkClearValue GetClearValue(const TextureClearValue& clearValue);
        VkComponentMapping GetSwizzle(VkFormat format);
        VkImageViewType GetViewType(TextureType type);
        VkAttachmentLoadOp GetLoadOp(LoadOp loadOp);
        VkAttachmentStoreOp GetStoreOp(StoreOp storeOp);
        VkCompareOp GetCompareOp(Comparison comparison);
        VkBorderColor GetBorderColor(BorderColor color);
        VkSamplerAddressMode GetSamplerAddressMode(WrapMode wrap);
        VkFilter GetFilterMode(FilterMode filter);
        VkDescriptorType GetDescriptorType(ShaderResourceType type);
        VkShaderStageFlagBits GetShaderStage(ShaderStage stage);
        VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples);
        VkVertexInputRate GetInputRate(InputRate inputRate);
        VkPolygonMode GetPolygonMode(PolygonMode mode);
        VkBlendOp GetBlendOp(BlendOp op);
        VkBlendFactor GetBlendFactor(BlendFactor factor, VkBlendFactor fallback);
        VkLogicOp GetLogicOp(LogicOp op);
        VkCullModeFlagBits GetCullMode(CullMode op);
        VkConservativeRasterizationModeEXT GetRasterMode(RasterMode mode, bool allowUnderEstimation);
        VkPrimitiveTopology GetTopology(Topology topology);
        VkFrontFace GetFrontFace(FrontFace face);
        VkRayTracingShaderGroupTypeKHR GetRayTracingStageGroupType(ShaderStage stage);
        VkImageAspectFlagBits GetFormatAspect(VkFormat format);
        
        VkPipelineStageFlags2 GetQueueFlagsStageCapabilities(VkQueueFlags flags);
        VkAccessFlags2 GetQueueFlagsAccessCapabilities(VkQueueFlags flags);
        VkPipelineStageFlags2 GetPipelineStageFlags(ShaderStageFlags flags);
        bool IsWriteAccess(VkAccessFlags2 access);
    }
}
