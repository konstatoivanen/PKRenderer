#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Systems/VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanDisposer.h"
#include "Rendering/VulkanRHI/Systems/VulkanSamplerCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanPipelineCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanStagingBufferCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanCommandBufferPool.h"
#include "Rendering/VulkanRHI/Systems/VulkanFrameBufferCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanLayoutCache.h"

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::GraphicsAPI;
    using namespace Systems;

    struct VulkanContextProperties
    {
        std::string appName;
        uint64_t garbagePruneDelay;
        const std::vector<const char*>* validationLayers;
        const std::vector<const char*>* contextualInstanceExtensions;
        const std::vector<const char*>* contextualDeviceExtensions;

        VulkanContextProperties(const std::string& appName = "Vulkan Engine",
            uint64_t garbagePruneDelay = 32ull,
            const std::vector<const char*>* validationLayers = nullptr,
            const std::vector<const char*>* contextualInstanceExtensions = nullptr,
            const std::vector<const char*>* contextualDeviceExtensions = nullptr) :
            appName(appName),
            garbagePruneDelay(garbagePruneDelay),
            validationLayers(validationLayers),
            contextualInstanceExtensions(contextualInstanceExtensions),
            contextualDeviceExtensions(contextualDeviceExtensions)
        {
        }
    };

    struct VulkanDriver : public GraphicsDriver
    {
        VulkanDriver(const VulkanContextProperties& properties);
        ~VulkanDriver();

        APIType GetAPI() const override final { return APIType::Vulkan; }

        CommandBuffer* GetPrimaryCommandBuffer() override final { return commandBufferPool->GetCurrent(); }

        void WaitForIdle() const override final { vkDeviceWaitIdle(device); }
        
        DriverMemoryInfo GetMemoryInfo() const override final;
        size_t GetBufferOffsetAlignment(BufferUsage usage) const override final;
        
        void GC() override final;

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        QueueFamilies queueFamilies;
        VmaAllocator allocator;
        VulkanContextProperties properties;
        VkPhysicalDeviceProperties physicalDeviceProperties;

        Scope<VulkanFrameBufferCache> frameBufferCache;
        Scope<VulkanCommandBufferPool> commandBufferPool;
        Scope<VulkanStagingBufferCache> stagingBufferCache;
        Scope<VulkanDescriptorCache> descriptorCache;
        Scope<VulkanPipelineCache> pipelineCache;
        Scope<VulkanSamplerCache> samplerCache;
        Scope<VulkanLayoutCache> layoutCache;
        Scope<VulkanDisposer> disposer;
    };
}