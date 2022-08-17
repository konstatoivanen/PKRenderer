#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/Utilities/VulkanExtensions.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Utilities;
    using namespace Systems;
    using namespace Objects;

    VulkanDriver::VulkanDriver(const VulkanContextProperties& properties) : properties(properties)
    {
        glfwInit();

        // Create a temporary hidden window so that we can query & select a physical device with surface present capabilities.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        auto temporaryWindow = glfwCreateWindow(32, 32, "Initialization Window", nullptr, nullptr);
        PK_THROW_ASSERT(temporaryWindow, "Failed To Create Window");

        uint32_t supportedApiVersion;
        VK_ASSERT_RESULT_CTX(vkEnumerateInstanceVersion(&supportedApiVersion), "Failed to query supported api version!");

        auto supportedMajor = VK_VERSION_MAJOR(supportedApiVersion);
        auto supportedMinor = VK_VERSION_MINOR(supportedApiVersion);

        if (properties.minApiVersionMajor > supportedMajor || properties.minApiVersionMinor > supportedMinor)
        {
            PK_THROW_ERROR("Vulkan version %i.%i required. Your driver only supports version %i.%i", properties.minApiVersionMajor, properties.minApiVersionMinor, supportedMajor, supportedMinor);
        }

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
        appInfo.apiVersion = supportedApiVersion;

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
        Utilities::VulkanBindExtensionMethods(instance);

        VK_ASSERT_RESULT_CTX(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger), "Failed to create debug messenger");

        VkSurfaceKHR temporarySurface;
        VK_ASSERT_RESULT_CTX(glfwCreateWindowSurface(instance, temporaryWindow, nullptr, &temporarySurface), "Failed to create window surface!");

        PhysicalDeviceRequirements physicalDeviceRequirements{};
        physicalDeviceRequirements.versionMajor = supportedMajor;
        physicalDeviceRequirements.versionMinor = supportedMinor;
        physicalDeviceRequirements.features.alphaToOne = VK_TRUE;
        physicalDeviceRequirements.features.shaderImageGatherExtended = VK_TRUE;
        physicalDeviceRequirements.features.sparseBinding = VK_TRUE;
        physicalDeviceRequirements.features.sparseResidencyBuffer = VK_TRUE;
        physicalDeviceRequirements.features.samplerAnisotropy = VK_TRUE;
        physicalDeviceRequirements.features.multiViewport = VK_TRUE;
        physicalDeviceRequirements.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
        physicalDeviceRequirements.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
        physicalDeviceRequirements.features.shaderFloat64 = VK_TRUE;
        physicalDeviceRequirements.features.shaderInt16 = VK_TRUE;
        physicalDeviceRequirements.features.shaderInt64 = VK_TRUE;
        physicalDeviceRequirements.features.imageCubeArray = VK_TRUE;
        physicalDeviceRequirements.features.fragmentStoresAndAtomics = VK_TRUE;
        physicalDeviceRequirements.features.multiDrawIndirect = VK_TRUE;
        physicalDeviceRequirements.features11.storageBuffer16BitAccess = VK_TRUE;
        physicalDeviceRequirements.features11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
        physicalDeviceRequirements.features11.storagePushConstant16 = VK_TRUE;
        physicalDeviceRequirements.features12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceRequirements.features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceRequirements.features12.runtimeDescriptorArray = VK_TRUE;
        physicalDeviceRequirements.features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        physicalDeviceRequirements.features12.descriptorBindingPartiallyBound = VK_TRUE;
        physicalDeviceRequirements.features12.scalarBlockLayout = VK_TRUE;
        physicalDeviceRequirements.features12.shaderFloat16 = VK_TRUE;
        physicalDeviceRequirements.features12.shaderInt8 = VK_TRUE;
        physicalDeviceRequirements.features12.shaderOutputViewportIndex = VK_TRUE;
        physicalDeviceRequirements.features12.shaderOutputLayer = VK_TRUE;
        physicalDeviceRequirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        physicalDeviceRequirements.deviceExtensions = properties.contextualDeviceExtensions;
        physicalDeviceRequirements.features12.pNext = &physicalDeviceRequirements.features11;
        Utilities::VulkanSelectPhysicalDevice(instance, temporarySurface, physicalDeviceRequirements, &physicalDevice, &queueFamilies);
        physicalDeviceProperties = Utilities::VulkanGetPhysicalDeviceProperties(physicalDevice);

        float queuePriority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = 
        { 
            queueFamilies[QueueType::Graphics], 
            queueFamilies[QueueType::Compute], 
            queueFamilies[QueueType::Present] 
        };

        for (auto queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &physicalDeviceRequirements.features;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(properties.contextualDeviceExtensions->size());
        createInfo.ppEnabledExtensionNames = properties.contextualDeviceExtensions->data();
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = &physicalDeviceRequirements.features12;

        if (properties.validationLayers != nullptr && properties.validationLayers->size() > 0)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(properties.validationLayers->size());
            createInfo.ppEnabledLayerNames = properties.validationLayers->data();
        }

        VK_ASSERT_RESULT_CTX(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Failed to create logical device!");

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.vulkanApiVersion = Utilities::VulkanGetPhysicalDeviceProperties(physicalDevice).apiVersion;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        VK_ASSERT_RESULT_CTX(vmaCreateAllocator(&allocatorInfo, &allocator), "Failed to create a VMA allocator!");

        vkDestroySurfaceKHR(instance, temporarySurface, nullptr);
        glfwDestroyWindow(temporaryWindow);

        frameBufferCache = CreateScope<VulkanFrameBufferCache>(device, properties.garbagePruneDelay);
        stagingBufferCache = CreateScope<VulkanStagingBufferCache>(device, allocator, properties.garbagePruneDelay);
        pipelineCache = CreateScope<VulkanPipelineCache>(device, properties.workingDirectory, properties.garbagePruneDelay);
        samplerCache = CreateScope<VulkanSamplerCache>(device);
        layoutCache = CreateScope<VulkanLayoutCache>(device);
        disposer = CreateScope<VulkanDisposer>();
        descriptorCache = CreateScope<VulkanDescriptorCache>(device, 4, 100ull,
                                                             std::initializer_list<std::pair<const VkDescriptorType, size_t>>({
                                                                 { VK_DESCRIPTOR_TYPE_SAMPLER, 100ull },
                                                                 { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100ull },
                                                                 { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100ull },
                                                             }));

        commandBufferPool = CreateScope<VulkanCommandBufferPool>(device,
                                                                 VulkanSystemContext
                                                                 {
                                                                     descriptorCache.get(),
                                                                     pipelineCache.get(),
                                                                     samplerCache.get(),
                                                                     frameBufferCache.get(),
                                                                     stagingBufferCache.get(),
                                                                     disposer.get()
                                                                 }, 
                                                                 queueFamilies[QueueType::Graphics]);
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
        layoutCache = nullptr;

        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
    }

    DriverMemoryInfo VulkanDriver::GetMemoryInfo() const
    {
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS]{};
        vmaGetHeapBudgets(allocator, budgets);

        size_t totalUsed = 0ull;
        size_t totalAvailable = 0ull;

        for (auto i = 0u; i < VK_MAX_MEMORY_HEAPS; ++i)
        {
            totalUsed += budgets[i].usage;
            totalAvailable += budgets[i].budget;
        }

        VmaTotalStatistics stats{};
        vmaCalculateStatistics(allocator, &stats);

        DriverMemoryInfo info{};
        info.blockCount = stats.total.statistics.blockCount;
        info.allocationCount = stats.total.statistics.allocationCount;
        info.unusedRangeCount = stats.total.unusedRangeCount;
        info.usedBytes = totalUsed;
        info.unusedBytes = totalAvailable - totalUsed;
        info.allocationSizeMin = stats.total.allocationSizeMin;
        info.allocationSizeAvg = stats.total.allocationSizeMin + (stats.total.allocationSizeMax - stats.total.allocationSizeMin) / 2;
        info.allocationSizeMax = stats.total.allocationSizeMax;
        info.unusedRangeSizeMin = stats.total.unusedRangeSizeMin;
        info.unusedRangeSizeAvg = stats.total.unusedRangeSizeMin + (stats.total.unusedRangeSizeMax - stats.total.unusedRangeSizeMin) / 2;
        info.unusedRangeSizeMax = stats.total.unusedRangeSizeMax;
        return info;
    }

    size_t VulkanDriver::GetBufferOffsetAlignment(BufferUsage usage) const
    {
        if ((usage & BufferUsage::Storage) != 0)
        {
            return physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
        }

        if ((usage & BufferUsage::Constant) != 0)
        {
            return physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
        }

        return sizeof(char);
    }

    void VulkanDriver::GC()
    {
        stagingBufferCache->Prune();
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


