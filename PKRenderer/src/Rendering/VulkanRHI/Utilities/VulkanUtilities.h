#pragma once
#include "Core/Services/Log.h"
#include "VulkanStructs.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Structs/Descriptors.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::Utilities
{
    #define VK_ASSERT_RESULT_CTX(cmd, ctx)                                                                                  \
            {                                                                                                               \
                auto result = cmd;                                                                                          \
                                                                                                                            \
                if (result != VK_SUCCESS)                                                                                   \
                {                                                                                                           \
                    PK_THROW_ERROR(ctx " (%s)", PK::Rendering::VulkanRHI::Utilities::VulkanResultToString(result).c_str()); \
                }                                                                                                           \
            }                                                                                                               \

    #define VK_ASSERT_RESULT(cmd) VK_ASSERT_RESULT_CTX(cmd, "VK COMMAND FAILED! ");
    #define VK_THROW_RESULT(result) PK_THROW_ERROR(PK::Rendering::VulkanRHI::Utilities::VulkanResultToString(result).c_str())

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

    VkAccelerationStructureBuildSizesInfoKHR VulkanGetAccelerationBuildSizesInfo(VkDevice device, const VkAccelerationStructureGeometryKHR& geometry, VkAccelerationStructureTypeKHR type, uint32_t primitiveCount);
    std::string VulkanResultToString(VkResult result);

    VkImageSubresourceRange VulkanConvertRange(const Structs::TextureViewRange& viewRange, VkImageAspectFlags aspect);
    Structs::TextureViewRange VulkanConvertRange(VkImageSubresourceRange resourceRange);
}