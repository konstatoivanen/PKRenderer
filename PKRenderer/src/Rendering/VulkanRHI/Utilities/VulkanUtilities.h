#pragma once
#include "Core/Services/Log.h"
#include "VulkanStructs.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::Utilities
{
    #define VK_ASSERT_RESULT(cmd) if ((cmd) != VK_SUCCESS) PK_THROW_ERROR("Vulkan Command Failed!");
    #define VK_ASSERT_RESULT_CTX(cmd, ctx) if ((cmd) != VK_SUCCESS) PK_THROW_ERROR(ctx);

    std::vector<VkLayerProperties> VulkanGetInstanceLayerProperties();
    std::vector<VkExtensionProperties> VulkanGetInstanceExtensions();
    std::vector<VkPhysicalDevice> VulkanGetPhysicalDevices(VkInstance instance);
    std::vector<VkExtensionProperties> VulkanGetPhysicalDeviceExtensionProperties(VkPhysicalDevice device);
    std::vector<VkQueueFamilyProperties> VulkanGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device);
    std::vector<const char*> VulkanGetRequiredInstanceExtensions(const std::vector<const char*>* contextualExtensions);
    std::vector<VkSurfaceFormatKHR> VulkanGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    std::vector<VkPresentModeKHR> VulkanGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VkPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device);
    QueueFamilies VulkanGetPhysicalDeviceQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

    bool VulkanValidateInstanceExtensions(const std::vector<const char*>* extensions);
    bool VulkanValidatePhysicalDeviceExtensions(VkPhysicalDevice device, const std::vector<const char*>* extensions);
    bool VulkanValidateValidationLayers(const std::vector<const char*>* validationLayers);

    void VulkanSelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* device, QueueFamilies* queueFamilies);
    VkExtent2D VulkanSelectSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& desiredExtent);
    VkSurfaceFormatKHR VulkanSelectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat desiredFormat, VkColorSpaceKHR desiredColorSpace);
    VkPresentModeKHR VulkanSelectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR desiredPresentMode);
}