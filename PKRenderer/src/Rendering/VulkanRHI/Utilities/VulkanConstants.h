#pragma once
#include "PrecompiledHeader.h"
#include "vulkan/vulkan.h"

namespace PK::Rendering::VulkanRHI
{
    constexpr const static uint32_t PK_DESIRED_SWAP_CHAIN_IMAGE_COUNT = 4;
    constexpr static const int PK_MAX_FRAMES_IN_FLIGHT = 2;
    constexpr static const int PK_MAX_RENDER_TARGETS = 8;
    constexpr static const uint32_t PK_MAX_DESCRIPTOR_SETS = 4;
    constexpr static const uint32_t PK_MAX_DESCRIPTORS_PER_SET = 16;
    constexpr const static uint32_t PK_QUEUE_FAMILY_COUNT = 2;
    constexpr const static uint32_t PK_INVALID_QUEUE_FAMILY = 0xFFFFFFFF;
    constexpr const static uint32_t PK_MAX_VERTEX_ATTRIBUTES = 8;

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

    constexpr static const char* PK_DEBUG_VERTEX_SHADER =
        "#version 450                                                               \n"
        "layout(set = 0, binding = 0) uniform UniformBufferObject                   \n"
        "{                                                                          \n"
        "    mat4 model;                                                            \n"
        "    mat4 viewproj;                                                         \n"
        "} ubo;                                                                     \n"
        "                                                                           \n"
        "layout(location = 0) in vec3 inPosition;                                   \n"
        "layout(location = 1) in vec2 inTexcoord;                                   \n"
        "layout(location = 2) in vec3 inColor;                                      \n"
        "layout(location = 0) out vec3 fragColor;                                   \n"
        "layout(location = 1) out vec2 texCoord;                                    \n"
        "                                                                           \n"
        "void main()                                                                \n"
        "{                                                                          \n"
        "    gl_Position = ubo.viewproj * ubo.model * vec4(inPosition, 1.0);        \n"
        "    fragColor = inColor;                                                   \n"
        "    texCoord = inTexcoord;                                                 \n"
        "}                                                                          \n";

    constexpr static const char* PK_DEBUG_FRAGMENT_SHADER =
        "#version 450                                                            \n"
        "                                                                        \n"
        "layout(location = 0) in vec3 fragColor;                                 \n"
        "layout(location = 1) in vec2 texCoord;                                  \n"
        "layout(set = 0, binding = 1) uniform sampler2D tex1;                    \n"
        "                                                                        \n"
        "layout(location = 0) out vec4 outColor;                                 \n"
        "                                                                        \n"
        "void main()                                                             \n"
        "{                                                                       \n"
        "    outColor = vec4(texture(tex1,texCoord).xyz * fragColor, 1.0);       \n"
        "}                                                                       \n";
}