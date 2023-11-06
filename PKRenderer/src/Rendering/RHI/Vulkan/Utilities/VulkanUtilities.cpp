#include "PrecompiledHeader.h"
#include <gfx.h>
#include <vulkan/vk_enum_string_helper.h>
#include "Core/Services/Log.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanExtensions.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanPhysicalDeviceRequirements.h"
#include "VulkanUtilities.h"

PFN_vkSetDebugUtilsObjectNameEXT pkfn_vkSetDebugUtilsObjectNameEXT = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT pkfn_vkSetDebugUtilsObjectTagEXT = nullptr;
PFN_vkQueueBeginDebugUtilsLabelEXT pkfn_vkQueueBeginDebugUtilsLabelEXT = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT pkfn_vkQueueEndDebugUtilsLabelEXT = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT pkfn_vkQueueInsertDebugUtilsLabelEXT = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT pkfn_vkCmdBeginDebugUtilsLabelEXT = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT pkfn_vkCmdEndDebugUtilsLabelEXT = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT pkfn_vkCmdInsertDebugUtilsLabelEXT = nullptr;
PFN_vkCreateDebugUtilsMessengerEXT pkfn_vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT pkfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkSubmitDebugUtilsMessageEXT pkfn_vkSubmitDebugUtilsMessageEXT = nullptr;

PFN_vkCreateAccelerationStructureKHR pkfn_vkCreateAccelerationStructureKHR = nullptr;
PFN_vkDestroyAccelerationStructureKHR pkfn_vkDestroyAccelerationStructureKHR = nullptr;
PFN_vkCmdSetRayTracingPipelineStackSizeKHR pkfn_vkCmdSetRayTracingPipelineStackSizeKHR = nullptr;
PFN_vkCmdTraceRaysIndirectKHR pkfn_vkCmdTraceRaysIndirectKHR = nullptr;
PFN_vkCmdTraceRaysKHR pkfn_vkCmdTraceRaysKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR pkfn_vkCreateRayTracingPipelinesKHR = nullptr;
PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR pkfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR pkfn_vkGetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkGetRayTracingShaderGroupStackSizeKHR pkfn_vkGetRayTracingShaderGroupStackSizeKHR = nullptr;
PFN_vkGetAccelerationStructureDeviceAddressKHR pkfn_vkGetAccelerationStructureDeviceAddressKHR = nullptr;
PFN_vkGetAccelerationStructureBuildSizesKHR pkfn_vkGetAccelerationStructureBuildSizesKHR = nullptr;
PFN_vkCmdBuildAccelerationStructuresKHR pkfn_vkCmdBuildAccelerationStructuresKHR = nullptr;
PFN_vkCmdCopyAccelerationStructureKHR pkfn_vkCmdCopyAccelerationStructureKHR = nullptr;
PFN_vkCmdPipelineBarrier2KHR pkfn_vkCmdPipelineBarrier2KHR = nullptr;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;

PFN_vkCmdDrawMeshTasksEXT pkfn_vkCmdDrawMeshTasksEXT = nullptr;
PFN_vkCmdDrawMeshTasksIndirectEXT pkfn_vkCmdDrawMeshTasksIndirectEXT = nullptr;
PFN_vkCmdDrawMeshTasksIndirectCountEXT pkfn_vkCmdDrawMeshTasksIndirectCountEXT = nullptr;

namespace PK::Rendering::RHI::Vulkan::Utilities
{
    void VulkanBindExtensionMethods(VkInstance instance)
    {
        pkfn_vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
        pkfn_vkSetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
        pkfn_vkQueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueBeginDebugUtilsLabelEXT");
        pkfn_vkQueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueEndDebugUtilsLabelEXT");
        pkfn_vkQueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkQueueInsertDebugUtilsLabelEXT");
        pkfn_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
        pkfn_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
        pkfn_vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
        pkfn_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        pkfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        pkfn_vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

        pkfn_vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCreateAccelerationStructureKHR");
        pkfn_vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkDestroyAccelerationStructureKHR");
        pkfn_vkCmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)vkGetInstanceProcAddr(instance, "vkCmdSetRayTracingPipelineStackSizeKHR");
        pkfn_vkCmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysIndirectKHR");
        pkfn_vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(instance, "vkCmdTraceRaysKHR");
        pkfn_vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(instance, "vkCreateRayTracingPipelinesKHR");
        pkfn_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
        pkfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR");
        pkfn_vkGetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupStackSizeKHR");
        pkfn_vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureDeviceAddressKHR");
        pkfn_vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(instance, "vkGetAccelerationStructureBuildSizesKHR");
        pkfn_vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(instance, "vkCmdBuildAccelerationStructuresKHR");
        pkfn_vkCmdCopyAccelerationStructureKHR = (PFN_vkCmdCopyAccelerationStructureKHR)vkGetInstanceProcAddr(instance, "vkCmdCopyAccelerationStructureKHR");
        pkfn_vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(instance, "vkGetRayTracingShaderGroupHandlesKHR");
        pkfn_vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetInstanceProcAddr(instance, "vkCmdWriteAccelerationStructuresPropertiesKHR");
        pkfn_vkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR");
        
        pkfn_vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksEXT");
        pkfn_vkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectEXT");
        pkfn_vkCmdDrawMeshTasksIndirectCountEXT = (PFN_vkCmdDrawMeshTasksIndirectCountEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectCountEXT");
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

    VulkanPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties2 deviceProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterizationProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };
        VkPhysicalDeviceSubgroupProperties subgroupProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };
        deviceProperties.pNext = &accelerationStructureProperties;
        accelerationStructureProperties.pNext = &rayTracingProperties;
        rayTracingProperties.pNext = &conservativeRasterizationProperties;
        conservativeRasterizationProperties.pNext = &subgroupProperties;

        vkGetPhysicalDeviceProperties2(device, &deviceProperties);

        VulkanPhysicalDeviceProperties returnProperties;
        returnProperties.properties = deviceProperties.properties;
        returnProperties.accelerationStructureProperties = accelerationStructureProperties;
        returnProperties.rayTracingProperties = rayTracingProperties;
        returnProperties.conservativeRasterizationProperties = conservativeRasterizationProperties;
        returnProperties.subgroupProperties = subgroupProperties;
        return returnProperties;
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
        auto availableExtensions = PK::Rendering::RHI::Vulkan::Utilities::VulkanGetPhysicalDeviceExtensionProperties(device);

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

        auto availableLayers = PK::Rendering::RHI::Vulkan::Utilities::VulkanGetInstanceLayerProperties();

        std::set<std::string> requiredLayers(validationLayers->begin(), validationLayers->end());

        for (const auto& layer : availableLayers)
        {
            requiredLayers.erase(layer.layerName);
        }

        return requiredLayers.empty();
    }

    bool VulkanIsPresentSupported(VkPhysicalDevice physicalDevice, uint32_t familyIndex, VkSurfaceKHR surface)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupport);
        return presentSupport;
    }

    void VulkanSelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* selectedDevice)
    {
        auto devices = VulkanGetPhysicalDevices(instance);
        *selectedDevice = VK_NULL_HANDLE;

        for (auto& device : devices)
        {
            auto properties = VulkanGetPhysicalDeviceProperties(device);
            auto versionMajor = VK_API_VERSION_MAJOR(properties.properties.apiVersion);
            auto versionMinor = VK_API_VERSION_MINOR(properties.properties.apiVersion);

            if (versionMajor < requirements.versionMajor)
            {
                continue;
            }

            if (versionMajor == requirements.versionMajor && versionMinor < requirements.versionMinor)
            {
                continue;
            }

            auto queueFamilies = VulkanGetPhysicalDeviceQueueFamilyProperties(device);
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

            auto hasPresent = false;
            auto queueMask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT;

            for (auto i = 0u; i < queueFamilies.size(); ++i)
            {
                auto& family = queueFamilies.at(i);
                queueMask &= ~family.queueFlags;
                hasPresent |= VulkanIsPresentSupported(device, i, surface);
            }

            if (properties.properties.deviceType != requirements.deviceType ||
                !extensionSupported ||
                !swapChainSupported ||
                !hasPresent ||
                queueMask != 0u)
            {
                continue;
            }

            auto& requiredFeatures = requirements.features;
            VulkanPhysicalDeviceFeatures features{};
            vkGetPhysicalDeviceFeatures2(device, &features.vk10);

            if (!VulkanPhysicalDeviceFeatures::CheckRequirements(requirements.features, features))
            {
                continue;
            }

            PK_LOG_NEWLINE();
            PK_LOG_INFO(" Selected Physical Device '%s' from '%i' Physical Devices:", properties.properties.deviceName, devices.size());
            PK_LOG_INFO("   Vendor: %i", properties.properties.vendorID);
            PK_LOG_INFO("   Device: %i", properties.properties.deviceID);
            PK_LOG_INFO("   Driver: %i", properties.properties.driverVersion);
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

    VkAccelerationStructureBuildSizesInfoKHR VulkanGetAccelerationBuildSizesInfo(VkDevice device, const VkAccelerationStructureBuildGeometryInfoKHR info, uint32_t primitiveCount)
    {
        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        pkfn_vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info, &primitiveCount, &accelerationStructureBuildSizesInfo);
        return accelerationStructureBuildSizesInfo;
    }

    std::string VulkanResultToString(VkResult result)
    {
        switch (result)
        {
            case VK_SUCCESS: return "VK_SUCCESS";
            case VK_NOT_READY: return "VK_NOT_READY";
            case VK_TIMEOUT: return "VK_TIMEOUT";
            case VK_EVENT_SET: return "VK_EVENT_SET";
            case VK_EVENT_RESET: return "VK_EVENT_RESET";
            case VK_INCOMPLETE: return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
            case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
            case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
            case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
            case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
            case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
            case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
#ifdef VK_ENABLE_BETA_EXTENSIONS
            case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
            case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
            case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
            case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
            case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
            case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
#endif
            case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
            case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
            case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
            case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
            case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
            case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
            case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
            case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
            case VK_RESULT_MAX_ENUM: return "VK_RESULT_MAX_ENUM";
            default: return "VK_RESULT_INVALID_ENUM";
        }
    }

    VkImageSubresourceRange VulkanConvertRange(const TextureViewRange& viewRange, VkImageAspectFlags aspect)
    {
        return
        {
            aspect,                     //aspectMask
            (uint32_t)viewRange.level,  //baseMipLevel
            (uint32_t)viewRange.levels, //levelCount
            (uint32_t)viewRange.layer,  //baseArrayLayer
            (uint32_t)viewRange.layers  //layerCount
        };
    }

    TextureViewRange VulkanConvertRange(const VkImageSubresourceRange& resourceRange)
    {
        return
        {
            (uint16_t)resourceRange.baseMipLevel,     //level
            (uint16_t)resourceRange.baseArrayLayer,   //layer
            (uint16_t)resourceRange.levelCount & 0x7FFFu,       //levels
            (uint16_t)resourceRange.layerCount & 0x7FFFu        //layers
        };
    }

    void VulkanSetObjectDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        nameInfo.pNext = nullptr;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = objectHandle;
        nameInfo.pObjectName = name;
        vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
        PK_LOG_VERBOSE("New: %s, %s", string_VkObjectType(nameInfo.objectType), name);
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