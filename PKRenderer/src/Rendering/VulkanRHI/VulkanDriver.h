#pragma once
#include "PrecompiledHeader.h"
#include "Core/NoCopy.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Systems/VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanDisposer.h"
#include "Rendering/VulkanRHI/Systems/VulkanSamplerCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanPipelineCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanStagingBufferCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanCommandBufferPool.h"
#include "Rendering/VulkanRHI/Systems/VulkanFrameBufferCache.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::GraphicsAPI;
    using namespace Systems;

    struct VulkanContextProperties
    {
        std::string appName;
        const std::vector<const char*>* validationLayers;
        const std::vector<const char*>* contextualInstanceExtensions;
        const std::vector<const char*>* contextualDeviceExtensions;

        uint32_t apiVersionMajor;
        uint32_t apiVersionMinor;

        VulkanContextProperties(const std::string& appName = "Vulkan Engine",
            const std::vector<const char*>* validationLayers = nullptr,
            const std::vector<const char*>* contextualInstanceExtensions = nullptr,
            const std::vector<const char*>* contextualDeviceExtensions = nullptr,
            uint32_t apiVersionMajor = 1,
            uint32_t apiVersionMinor = 0) :
            appName(appName),
            validationLayers(validationLayers),
            contextualInstanceExtensions(contextualInstanceExtensions),
            contextualDeviceExtensions(contextualDeviceExtensions),
            apiVersionMajor(apiVersionMajor),
            apiVersionMinor(apiVersionMinor)
        {
        }
    };

    struct VulkanDriver : public GraphicsDriver
    {
        VulkanDriver(const VulkanContextProperties& properties);
        ~VulkanDriver();

        void PruneCaches();

        APIType GetAPI() const override { return APIType::Vulkan; }

        inline void WaitForIdle() const { vkDeviceWaitIdle(device); }

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

        Scope<VulkanFrameBufferCache> frameBufferCache;
        Scope<VulkanCommandBufferPool> commandBufferPool;
        Scope<VulkanStagingBufferCache> stagingBufferCache;
        Scope<VulkanDescriptorCache> descriptorCache;
        Scope<VulkanPipelineCache> pipelineCache;
        Scope<VulkanSamplerCache> samplerCache;
        Scope<VulkanDisposer> disposer;
    };
}