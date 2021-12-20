#include "PrecompiledHeader.h"
#include "VulkanUtilities.h"
#include "Math/PKMath.h"
#include "Utilities/Log.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI::Utilities
{
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

    VkPhysicalDeviceFeatures VulkanGetPhysicalDeviceFeatures(VkPhysicalDevice device)
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        return deviceFeatures;
    }

    QueueFamilies VulkanGetPhysicalDeviceQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilies indices{};

        auto queueFamilies = PK::Rendering::VulkanRHI::Utilities::VulkanGetPhysicalDeviceQueueFamilyProperties(device);

        for (auto i = 0u; i < queueFamilies.size(); ++i)
        {
            if ((queueFamilies.at(i).queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                indices[QueueType::Graphics].index = i;
            
                // @TODO consider a separate queue for compute
                indices[QueueType::Compute].index = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices[QueueType::Present].index = i;
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


    void VulkanSelectPhysicalDevice(VkInstance instance,  VkSurfaceKHR surface, const PhysicalDeviceRequirements& requirements, VkPhysicalDevice* selectedDevice, QueueFamilies* queueFamilies)
    {
        auto devices = VulkanGetPhysicalDevices(instance);
        *selectedDevice = VK_NULL_HANDLE;
        *queueFamilies = QueueFamilies();

        for (auto& device : devices)
        {
            auto properties = VulkanGetPhysicalDeviceProperties(device);
            auto features = VulkanGetPhysicalDeviceFeatures(device);

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

            if (properties.deviceType == requirements.deviceType &&
                (features.alphaToOne || !requirements.alphaToOne) &&
                (features.shaderImageGatherExtended || !requirements.shaderImageGatherExtended) &&
                (features.sparseBinding || !requirements.sparseBinding) &&
                (features.samplerAnisotropy || !requirements.samplerAnisotropy) &&
                (features.multiViewport || !requirements.multiViewport) &&
                extensionSupported &&
                swapChainSupported &&
                queueFamilies->HasIndices())
            {
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