#pragma once
#include "PrecompiledHeader.h"
#include "vulkan/vulkan.h"

namespace PK::Rendering::VulkanRHI
{
    constexpr const static uint32_t PK_QUEUE_FAMILY_COUNT = 2;
    constexpr const static uint32_t PK_INVALID_QUEUE_FAMILY = 0xFFFFFFFF;

    constexpr static const int VK_REQUIRED_VERSION_MAJOR = 1;
    constexpr static const int VK_REQUIRED_VERSION_MINOR = 0;

    static const std::vector<const char*> PK_VALIDATION_LAYERS =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    static const std::vector<const char*> PK_INSTANCE_EXTENTIONS =
    {
        "VK_EXT_debug_utils",
    };

    static const std::vector<const char*> PK_DEVICE_EXTENTIONS =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}