#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "VulkanUtilities.h"
#include <gfx.h>

PFN_vkSetDebugUtilsObjectNameEXT pk_vkSetDebugUtilsObjectNameEXT = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT pk_vkSetDebugUtilsObjectTagEXT = nullptr;
PFN_vkQueueBeginDebugUtilsLabelEXT pk_vkQueueBeginDebugUtilsLabelEXT = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT pk_vkQueueEndDebugUtilsLabelEXT = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT pk_vkQueueInsertDebugUtilsLabelEXT = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT pk_vkCmdBeginDebugUtilsLabelEXT = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT pk_vkCmdEndDebugUtilsLabelEXT = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT pk_vkCmdInsertDebugUtilsLabelEXT = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT pk_vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT pk_vkDestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkSubmitDebugUtilsMessageEXT pk_vkSubmitDebugUtilsMessageEXT = nullptr;

PFN_vkCreateAccelerationStructureKHR pk_vkCreateAccelerationStructureKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR pk_vkDestroyAccelerationStructureKHR = nullptr;
PFN_vkCmdSetRayTracingPipelineStackSizeKHR pk_vkCmdSetRayTracingPipelineStackSizeKHR = nullptr;
PFN_vkCmdTraceRaysIndirectKHR pk_vkCmdTraceRaysIndirectKHR = nullptr;
PFN_vkCmdTraceRaysKHR pk_vkCmdTraceRaysKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR pk_vkCreateRayTracingPipelinesKHR = nullptr;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pk_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR pk_vkGetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupStackSizeKHR pk_vkGetRayTracingShaderGroupStackSizeKHR = nullptr;

namespace PK::Rendering::VulkanRHI::Utilities
{
    void VulkanBindExtensionMethods(VkInstance instance)
    {
        pk_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
        pk_vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
        pk_vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
        pk_vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
        pk_vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT");
        pk_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
        pk_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
        pk_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
        pk_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        pk_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        pk_vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

        pk_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR");
        pk_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR");
        pk_vkCmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)vkGetInstanceProcAddr(instance, "vkCmdSetRayTracingPipelineStackSizeKHR");
        pk_vkCmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysIndirectKHR");
        pk_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysKHR");
        pk_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR");
        pk_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
        pk_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR");
        pk_vkGetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupStackSizeKHR");
    }

    std::vector<VkLayerProperties> VulkanGetInstanceLayerProperties()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
        std::vector<VkLayerProperties> layerProperties(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());
    
        return layerProperties;
    }
    
    std::vector<VkExtensionProperties> VulkanGetInstanceExtensions()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    
        return extensions;
    }

    std::vector<VkPhysicalDevice> VulkanGetPhysicalDevices(VkInstance instance)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        return devices;
    }

    std::vector<VkExtensionProperties> VulkanGetPhysicalDeviceExtensionProperties(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

        return extensions;
    }

    std::vector<VkQueueFamilyProperties> VulkanGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        return queueFamilies;
    }

    std::vector<const char*> VulkanGetRequiredInstanceExtensions(const std::vector<const char*>* contextualExtensions)
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (contextualExtensions != nullptr && contextualExtensions->size() > 0)
        {
            extensions.insert(std::end(extensions), std::begin(*contextualExtensions), std::end(*contextualExtensions));
        }

        return extensions;
    }

    std::vector<VkSurfaceFormatKHR> VulkanGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        std::vector<VkSurfaceFormatKHR> formats;
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        return formats;
    }

    std::vector<VkPresentModeKHR> VulkanGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        std::vector<VkPresentModeKHR> presentModes;
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

        return presentModes;
    }

    VkPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        return deviceProperties;
    }

    QueueFamilies VulkanGetPhysicalDeviceQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilies indices{};

        auto queueFamilies = PK::Rendering::VulkanRHI::Utilities::VulkanGetPhysicalDeviceQueueFamilyProperties(device);

        for (auto i = 0u; i < queueFamilies.size(); ++i)
        {
            if ((queueFamilies.at(i).queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                indices[QueueType::Graphics] = i;
                indices[QueueType::Compute] = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices[QueueType::Present] = i;
            }
        }

        return indices;
    }

    void VulkanDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        PK_THROW_ASSERT(func != nullptr, "Could not bind vkDestroyDebugUtilsMessengerEXT");
        func(instance, debugMessenger, pAllocator);
    }

    VkResult VulkanCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        PK_THROW_ASSERT(func != nullptr, "Could not bind vkCreateDebugUtilsMessengerEXT");
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }

    void VulkanCmdBeginDebugUtilsLabelEXT(VkInstance instance, VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* labelInfo)
    {
        auto pfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
        pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, labelInfo);
    }

    void VulkanCmdEndDebugUtilsLabelEXT(VkInstance instance, VkCommandBuffer commandBuffer)
    {
        auto pfnCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
        pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
    }


    bool VulkanValidateInstanceExtensions(const std::vector<const char*>* extensions)
    {
        if (extensions == nullptr || extensions->size() == 0)
        {
            return true;
        }

        auto availableExtensions = VulkanGetInstanceExtensions();

        std::set<std::string> requiredExtensions(extensions->begin(), extensions->end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    bool VulkanValidatePhysicalDeviceExtensions(VkPhysicalDevice device, const std::vector<const char*>* extensions)
    {
        auto availableExtensions = PK::Rendering::VulkanRHI::Utilities::VulkanGetPhysicalDeviceExtensionProperties(device);

        std::set<std::string> requiredExtensions(extensions->begin(), extensions->end());

        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    bool VulkanValidateValidationLayers(const std::vector<const char*>* validationLayers)
    {
        if (validationLayers == nullptr || validationLayers->size() == 0)
        {
            return true;
        }

        auto availableLayers = PK::Rendering::VulkanRHI::Utilities::VulkanGetInstanceLayerProperties();

        std::set<std::string> requiredLayers(validationLayers->begin(), validationLayers->end());

        for (const auto& layer : availableLayers)
        {
            requiredLayers.erase(layer.layerName);
        }

        return requiredLayers.empty();
    }

    template<typename TVal>
    static bool VulkanCheckRequirements(const TVal& values, const TVal& requirements, size_t offset, size_t count)
    {
        auto valuesPtr = reinterpret_cast<const VkBool32*>(reinterpret_cast<const char*>(&values) + offset);
        auto requirementsPtr = reinterpret_cast<const VkBool32*>(reinterpret_cast<const char*>(&requirements) + offset);

        for (auto i = 0u; i < count; ++i)
        {
            if (!valuesPtr[i] && requirementsPtr[i])
            {
                return false;
            }
        }

        return true;
    }

    template<typename TVal, typename ... Args>
    static bool VulkanCheckRequirements(const TVal& values, const TVal& requirements, size_t offset, size_t count, Args&&... args)
    {
        if (!VulkanCheckRequirements(values, requirements, offset, count))
        {
            return false;
        }

        return VulkanCheckRequirements(args...);
    }

    void VulkanSelectPhysicalDevice(VkInstance instance,  VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* selectedDevice, QueueFamilies* queueFamilies)
    {
        auto devices = VulkanGetPhysicalDevices(instance);
        *selectedDevice = VK_NULL_HANDLE;
        *queueFamilies = QueueFamilies();

        for (auto& device : devices)
        {
            auto properties = VulkanGetPhysicalDeviceProperties(device);
            auto versionMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
            auto versionMinor = VK_API_VERSION_MINOR(properties.apiVersion);

            if (versionMajor < requirements.versionMajor)
            {
                continue;
            }
            
            if (versionMajor == requirements.versionMajor && versionMinor < requirements.versionMinor) 
            {
                continue;
            }

            *queueFamilies = VulkanGetPhysicalDeviceQueueFamilyIndices(device, surface);
            auto extensionSupported = VulkanValidatePhysicalDeviceExtensions(device, requirements.deviceExtensions);
            auto swapChainSupported = false;

            if (extensionSupported)
            {
                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

                swapChainSupported = presentModeCount > 0 && formatCount > 0;
            }

            auto hasQueueFamilies = true;

            for (auto i = 0; i < PK_QUEUE_FAMILY_COUNT; ++i)
            {
                if (queueFamilies->indices[i] == PK_INVALID_QUEUE_FAMILY)
                {
                    hasQueueFamilies = false;
                    break;
                }
            }

            if (properties.deviceType != requirements.deviceType ||
                !extensionSupported ||
                !swapChainSupported ||
                !hasQueueFamilies)
            {
                continue;
            }

            VulkanPhysicalDeviceFeatures features{};
            vkGetPhysicalDeviceFeatures2(device, &features.vk10);

            if (!VulkanCheckRequirements(
                features.vk10, requirements.features.vk10, offsetof(VkPhysicalDeviceFeatures2, features), 55,
                features.vk11, requirements.features.vk11, offsetof(VkPhysicalDeviceVulkan11Features, storageBuffer16BitAccess), 12,
                features.vk12, requirements.features.vk12, offsetof(VkPhysicalDeviceVulkan12Features, samplerMirrorClampToEdge), 47,
                features.accelerationStructure, requirements.features.accelerationStructure, offsetof(VkPhysicalDeviceAccelerationStructureFeaturesKHR, accelerationStructure), 5,
                features.rayTracingPipeline, requirements.features.rayTracingPipeline, offsetof(VkPhysicalDeviceRayTracingPipelineFeaturesKHR, rayTracingPipeline), 5))
            {
                continue;
            }

            PK_LOG_NEWLINE();
            PK_LOG_HEADER(" Selected Physical Device '%s' from '%i' physical devices. ", properties.deviceName, devices.size());
            PK_LOG_INFO("   Vendor: %i", properties.vendorID);
            PK_LOG_INFO("   Device: %i", properties.deviceID);
            PK_LOG_INFO("   Driver: %i", properties.driverVersion);
            PK_LOG_INFO("   API VER: %i.%i", versionMajor, versionMinor);
            PK_LOG_NEWLINE();

            *selectedDevice = device;
            return;
        }

        PK_THROW_ERROR("Could not find a suitable vulkan physical device!");
    }

    VkSurfaceFormatKHR VulkanSelectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat desiredFormat, VkColorSpaceKHR desiredColorSpace)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == desiredFormat && availableFormat.colorSpace == desiredColorSpace)
            {
                return availableFormat;
            }
        }

        return availableFormats.at(0);
    }

    VkPresentModeKHR VulkanSelectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR desiredPresentMode)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == desiredPresentMode)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSelectSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& desiredExtent)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }

        VkExtent2D actualExtent = desiredExtent;
        actualExtent.width = glm::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = glm::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}