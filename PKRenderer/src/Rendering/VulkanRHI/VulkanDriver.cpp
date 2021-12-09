#include "PrecompiledHeader.h"
#include "VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Utilities/Log.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI
{
    VulkanDriver::VulkanDriver(const VulkanContextProperties& properties) : properties(properties)
    {
        glfwInit();

        // Create a temporary hidden window so that we can query & select a physical device with surface present capabilities.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        auto temporaryWindow = glfwCreateWindow(32, 32, "Initialization Window", nullptr, nullptr);
        PK_THROW_ASSERT(temporaryWindow, "Failed To Create Window");

        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.pfnUserCallback = VulkanDebugCallback;
        debugMessengerCreateInfo.pUserData = nullptr;

        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = properties.appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "PK Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_API_VERSION(0, properties.apiVersionMajor, properties.apiVersionMinor, 0);

        VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.pNext = &debugMessengerCreateInfo;

        auto instanceExtensions = Utilities::VulkanGetRequiredInstanceExtensions(properties.contextualInstanceExtensions);
        PK_THROW_ASSERT(Utilities::VulkanValidateInstanceExtensions(&instanceExtensions), "Trying to enable unavailable extentions!");
        PK_THROW_ASSERT(Utilities::VulkanValidateValidationLayers(properties.validationLayers), "Trying to enable unavailable validation layers!");

        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
        instanceCreateInfo.enabledLayerCount = 0;

        if (properties.validationLayers != nullptr && properties.validationLayers->size() > 0)
        {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(properties.validationLayers->size());
            instanceCreateInfo.ppEnabledLayerNames = properties.validationLayers->data();
        }

        VK_ASSERT_RESULT_CTX(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Failed to create vulkan instance!");
        VK_ASSERT_RESULT_CTX(Utilities::VulkanCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger), "Failed to create debug messenger");

        VkSurfaceKHR temporarySurface;
        VK_ASSERT_RESULT_CTX(glfwCreateWindowSurface(instance, temporaryWindow, nullptr, &temporarySurface), "Failed to create window surface!");

        PhysicalDeviceRequirements physicalDeviceRequirements{};
        physicalDeviceRequirements.versionMajor = properties.apiVersionMajor;
        physicalDeviceRequirements.versionMinor = properties.apiVersionMinor;
        physicalDeviceRequirements.alphaToOne = true;
        physicalDeviceRequirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        physicalDeviceRequirements.deviceExtensions = properties.contextualDeviceExtensions;
        Utilities::VulkanSelectPhysicalDevice(instance, temporarySurface, physicalDeviceRequirements, &physicalDevice, &queueFamilies);

        float queuePriority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = 
        { 
            queueFamilies[QueueType::Graphics].index, 
            queueFamilies[QueueType::Compute].index, 
            queueFamilies[QueueType::Present].index 
        };

        for (auto queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(properties.contextualDeviceExtensions->size());
        createInfo.ppEnabledExtensionNames = properties.contextualDeviceExtensions->data();
        createInfo.enabledLayerCount = 0;

        if (properties.validationLayers != nullptr && properties.validationLayers->size() > 0)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(properties.validationLayers->size());
            createInfo.ppEnabledLayerNames = properties.validationLayers->data();
        }

        VK_ASSERT_RESULT_CTX(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Failed to create logical device!");

        for (auto i = 0; i < PK_QUEUE_FAMILY_COUNT; ++i)
        {
            vkGetDeviceQueue(device, queueFamilies.queues[i].index, 0, &queueFamilies[(QueueType)i].queue);
        }

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.vulkanApiVersion = Utilities::VulkanGetPhysicalDeviceProperties(physicalDevice).apiVersion;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        VK_ASSERT_RESULT_CTX(vmaCreateAllocator(&allocatorInfo, &allocator), "Failed to create a VMA allocator!");

        vkDestroySurfaceKHR(instance, temporarySurface, nullptr);
        glfwDestroyWindow(temporaryWindow);

        frameBufferCache = CreateScope<VulkanFrameBufferCache>(device, 10);
        stagingBufferCache = CreateScope<VulkanStagingBufferCache>(allocator, 10);
        pipelineCache = CreateScope<VulkanPipelineCache>(device, 10);
        samplerCache = CreateScope<VulkanSamplerCache>(device);
        disposer = CreateScope<VulkanDisposer>();
        descriptorCache = CreateScope<VulkanDescriptorCache>(device, 10, 100ull,
                                                             std::initializer_list<std::pair<const VkDescriptorType, size_t>>({
                                                                 { VK_DESCRIPTOR_TYPE_SAMPLER, 100ull },
                                                                 { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100ull },
                                                                 { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100ull },
                                                             }));

        commandBufferPool = CreateScope<VulkanCommandBufferPool>(device, 
                                                                 VulkanRenderState
                                                                 (
                                                                     descriptorCache.get(), 
                                                                     pipelineCache.get(), 
                                                                     samplerCache.get(), 
                                                                     frameBufferCache.get(), 
                                                                     stagingBufferCache.get(), 
                                                                     disposer.get() 
                                                                 ), 
                                                                 queueFamilies[QueueType::Graphics].index);
    }

    VulkanDriver::~VulkanDriver()
    {
        vkDeviceWaitIdle(device);

        descriptorCache = nullptr;
        disposer = nullptr;
        samplerCache = nullptr;
        pipelineCache = nullptr;
        stagingBufferCache = nullptr;
        commandBufferPool = nullptr;
        frameBufferCache = nullptr;

        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
        Utilities::VulkanDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
    }

    void VulkanDriver::PruneCaches()
    {
        stagingBufferCache->Prune(false);
        pipelineCache->Prune();
        descriptorCache->Prune();
        disposer->Prune();
        frameBufferCache->Prune();
    }


    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDriver::VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                      VkDebugUtilsMessageTypeFlagsEXT messageType, 
                                                                      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                                                                      void* pUserData)
    {
        auto isValidationError = strstr(pCallbackData->pMessage, "Error") != nullptr;

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT || isValidationError)
        {
            PK_THROW_ERROR(pCallbackData->pMessage);
            return VK_FALSE;
        }

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            PK_LOG_WARNING(pCallbackData->pMessage);
            return VK_FALSE;
        }

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            PK_LOG_INFO(pCallbackData->pMessage);
            return VK_FALSE;
        }

        PK_LOG_VERBOSE(pCallbackData->pMessage);
        return VK_FALSE;
    }
}


