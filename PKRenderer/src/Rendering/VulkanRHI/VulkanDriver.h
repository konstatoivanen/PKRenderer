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

    struct VulkanDriver : public GraphicsDriver
    {
        VulkanDriver(const VulkanContextProperties& properties);
        ~VulkanDriver();

        Rendering::Structs::APIType GetAPI() const override final { return Rendering::Structs::APIType::Vulkan; }
        Rendering::Objects::CommandBuffer* GetPrimaryCommandBuffer() override final { return commandBufferPool->GetCurrent(); }

        void WaitForIdle() const override final { vkDeviceWaitIdle(device); }
        
        DriverMemoryInfo GetMemoryInfo() const override final;
        size_t GetBufferOffsetAlignment(Structs::BufferUsage usage) const override final;
        
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

        PK::Utilities::Scope<Systems::VulkanFrameBufferCache> frameBufferCache;
        PK::Utilities::Scope<Systems::VulkanCommandBufferPool> commandBufferPool;
        PK::Utilities::Scope<Systems::VulkanStagingBufferCache> stagingBufferCache;
        PK::Utilities::Scope<Systems::VulkanDescriptorCache> descriptorCache;
        PK::Utilities::Scope<Systems::VulkanPipelineCache> pipelineCache;
        PK::Utilities::Scope<Systems::VulkanSamplerCache> samplerCache;
        PK::Utilities::Scope<Systems::VulkanLayoutCache> layoutCache;
        PK::Utilities::Scope<Systems::VulkanDisposer> disposer;
    };
}