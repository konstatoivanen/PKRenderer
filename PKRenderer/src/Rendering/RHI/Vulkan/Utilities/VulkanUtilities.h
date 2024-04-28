#pragma once
#include "Core/CLI/Log.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanPhysicalDeviceRequirements.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::RHI::Vulkan::Utilities
{
    #define VK_ASSERT_RESULT_CTX(cmd, ctx)                                                                              \
    {                                                                                                                   \
        auto result = cmd;                                                                                              \
                                                                                                                        \
        if (result != VK_SUCCESS)                                                                                       \
        {                                                                                                               \
            PK_THROW_ERROR(ctx " (%s)", PK::Rendering::RHI::Vulkan::Utilities::VulkanResultToString(result).c_str());   \
        }                                                                                                               \
    }                                                                                                                   \

    #define VK_ASSERT_RESULT(cmd) VK_ASSERT_RESULT_CTX(cmd, "VK COMMAND FAILED! ");
    #define VK_THROW_RESULT(result) PK_THROW_ERROR(PK::Rendering::RHI::Vulkan::Utilities::VulkanResultToString(result).c_str())

    std::vector<VkLayerProperties> VulkanGetInstanceLayerProperties();
    std::vector<VkExtensionProperties> VulkanGetInstanceExtensions();
    std::vector<VkPhysicalDevice> VulkanGetPhysicalDevices(VkInstance instance);
    std::vector<VkExtensionProperties> VulkanGetPhysicalDeviceExtensionProperties(VkPhysicalDevice device);
    std::vector<VkQueueFamilyProperties> VulkanGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device);
    std::vector<const char*> VulkanGetRequiredInstanceExtensions(const std::vector<const char*>* contextualExtensions);
    std::vector<VkSurfaceFormatKHR> VulkanGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    std::vector<VkPresentModeKHR> VulkanGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    VulkanPhysicalDeviceProperties VulkanGetPhysicalDeviceProperties(VkPhysicalDevice device);

    bool VulkanValidateInstanceExtensions(const std::vector<const char*>* extensions);
    bool VulkanValidatePhysicalDeviceExtensions(VkPhysicalDevice device, const std::vector<const char*>* extensions);
    bool VulkanValidateValidationLayers(const std::vector<const char*>* validationLayers);
    bool VulkanIsPresentSupported(VkPhysicalDevice physicalDevice, uint32_t familyIndex, VkSurfaceKHR surface);

    void VulkanSelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanPhysicalDeviceRequirements& requirements, VkPhysicalDevice* device);
    VkExtent2D VulkanSelectSurfaceExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& desiredExtent);
    VkSurfaceFormatKHR VulkanSelectSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat desiredFormat, VkColorSpaceKHR desiredColorSpace);
    VkPresentModeKHR VulkanSelectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR desiredPresentMode);

    VkAccelerationStructureBuildSizesInfoKHR VulkanGetAccelerationBuildSizesInfo(VkDevice device, const VkAccelerationStructureBuildGeometryInfoKHR info, uint32_t primitiveCount);
    std::string VulkanResultToString(VkResult result);

    VkImageSubresourceRange VulkanConvertRange(const TextureViewRange& viewRange, VkImageAspectFlags aspect);
    TextureViewRange VulkanConvertRange(const VkImageSubresourceRange& resourceRange);

    void VulkanSetObjectDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name);
}