#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/Utilities/VulkanExtensions.h"
#include "Utilities/Handle.h"
#include "Rendering/VulkanRHI/Objects/VulkanBuffer.h"
#include "Rendering/VulkanRHI/Objects/VulkanTexture.h"
#include "Rendering/VulkanRHI/Objects/VulkanAccelerationStructure.h"
#include "Rendering/VulkanRHI/Objects/VulkanBindArray.h"
#include <gfx.h>

namespace PK::Rendering::VulkanRHI
{
    using namespace PK::Utilities;
    using namespace Rendering::Services;
    using namespace Services;
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
        apiVersion = supportedApiVersion;

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

        PK_LOG_NEWLINE();
        PK_LOG_HEADER(" Created Vulkan Instace With '%i' layers and '%i' extensions:", instanceCreateInfo.enabledLayerCount, instanceCreateInfo.enabledExtensionCount);

        for (auto i = 0u; i < instanceCreateInfo.enabledExtensionCount; ++i)
        {
            PK_LOG_INFO("   %s", instanceCreateInfo.ppEnabledExtensionNames[i]);
        }

        for (auto i = 0u; i < instanceCreateInfo.enabledLayerCount; ++i)
        {
            PK_LOG_INFO("   %s", instanceCreateInfo.ppEnabledLayerNames[i]);
        }

        VK_ASSERT_RESULT_CTX(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger), "Failed to create debug messenger");

        VkSurfaceKHR temporarySurface;
        VK_ASSERT_RESULT_CTX(glfwCreateWindowSurface(instance, temporaryWindow, nullptr, &temporarySurface), "Failed to create window surface!");

        VulkanPhysicalDeviceRequirements physicalDeviceRequirements{};
        physicalDeviceRequirements.versionMajor = supportedMajor;
        physicalDeviceRequirements.versionMinor = supportedMinor;
        physicalDeviceRequirements.features.vk10.features.alphaToOne = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.shaderImageGatherExtended = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.sparseBinding = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.sparseResidencyBuffer = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.samplerAnisotropy = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.multiViewport = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.shaderFloat64 = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.shaderInt16 = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.shaderInt64 = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.imageCubeArray = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.fragmentStoresAndAtomics = VK_TRUE;
        physicalDeviceRequirements.features.vk10.features.multiDrawIndirect = VK_TRUE;
        physicalDeviceRequirements.features.vk11.storageBuffer16BitAccess = VK_TRUE;
        physicalDeviceRequirements.features.vk11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
        physicalDeviceRequirements.features.vk11.storagePushConstant16 = VK_TRUE;
        physicalDeviceRequirements.features.vk12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceRequirements.features.vk12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceRequirements.features.vk12.runtimeDescriptorArray = VK_TRUE;
        physicalDeviceRequirements.features.vk12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        physicalDeviceRequirements.features.vk12.descriptorBindingPartiallyBound = VK_TRUE;
        physicalDeviceRequirements.features.vk12.scalarBlockLayout = VK_TRUE;
        physicalDeviceRequirements.features.vk12.shaderFloat16 = VK_TRUE;
        physicalDeviceRequirements.features.vk12.shaderInt8 = VK_TRUE;
        physicalDeviceRequirements.features.vk12.shaderOutputViewportIndex = VK_TRUE;
        physicalDeviceRequirements.features.vk12.shaderOutputLayer = VK_TRUE;
        physicalDeviceRequirements.features.vk12.bufferDeviceAddress = VK_TRUE;
        physicalDeviceRequirements.features.vk12.timelineSemaphore = VK_TRUE;
        physicalDeviceRequirements.features.accelerationStructure.accelerationStructure = VK_TRUE;
        physicalDeviceRequirements.features.rayTracingPipeline.rayTracingPipeline = VK_TRUE;
        physicalDeviceRequirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        physicalDeviceRequirements.deviceExtensions = properties.contextualDeviceExtensions;

        Utilities::VulkanSelectPhysicalDevice(instance, temporarySurface, physicalDeviceRequirements, &physicalDevice);
        physicalDeviceProperties = Utilities::VulkanGetPhysicalDeviceProperties(physicalDevice);

        VulkanQueueSet::Initializer queueInitializer(physicalDevice, temporarySurface);

        VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInitializer.createInfos.size());
        createInfo.pQueueCreateInfos = queueInitializer.createInfos.data();
        createInfo.pEnabledFeatures = nullptr;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(properties.contextualDeviceExtensions->size());
        createInfo.ppEnabledExtensionNames = properties.contextualDeviceExtensions->data();
        createInfo.enabledLayerCount = instanceCreateInfo.enabledLayerCount;
        createInfo.ppEnabledLayerNames = instanceCreateInfo.ppEnabledLayerNames;
        createInfo.pNext = &physicalDeviceRequirements.features.vk10;
        VK_ASSERT_RESULT_CTX(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Failed to create logical device!");

        PK_LOG_INFO(" Created Vulkan Device With '%i' extensions:", createInfo.enabledExtensionCount);

        for (auto i = 0u; i < createInfo.enabledExtensionCount; ++i)
        {
            PK_LOG_INFO("   %s", createInfo.ppEnabledExtensionNames[i]);
        }

        PK_LOG_NEWLINE();

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.vulkanApiVersion = physicalDeviceProperties.properties.apiVersion;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        VK_ASSERT_RESULT_CTX(vmaCreateAllocator(&allocatorInfo, &allocator), "Failed to create a VMA allocator!");

        vkDestroySurfaceKHR(instance, temporarySurface, nullptr);
        glfwDestroyWindow(temporaryWindow);


        frameBufferCache = CreateScope<VulkanFrameBufferCache>(device, properties.garbagePruneDelay);
        stagingBufferCache = CreateScope<VulkanStagingBufferCache>(device, allocator, properties.garbagePruneDelay);
        pipelineCache = CreateScope<VulkanPipelineCache>(device, properties.workingDirectory, physicalDeviceProperties, properties.garbagePruneDelay);
        samplerCache = CreateScope<VulkanSamplerCache>(device);
        layoutCache = CreateScope<VulkanLayoutCache>(device);
        disposer = CreateScope<Disposer>();
        descriptorCache = CreateScope<VulkanDescriptorCache>(device, 4, 100ull,
            std::initializer_list<std::pair<const VkDescriptorType, size_t>>({
                { VK_DESCRIPTOR_TYPE_SAMPLER, 100ull },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100ull },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100ull },
                { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10ull }
                }));

        queues = CreateScope<VulkanQueueSet>(device,
            queueInitializer,
            VulkanServiceContext
            {
                &globalResources,
                descriptorCache.get(),
                pipelineCache.get(),
                samplerCache.get(),
                frameBufferCache.get(),
                stagingBufferCache.get(),
                nullptr, // Assigned by queues
                disposer.get()
            });
    }

    VulkanDriver::~VulkanDriver()
    {
        vkDeviceWaitIdle(device);

        descriptorCache = nullptr;
        disposer = nullptr;
        samplerCache = nullptr;
        pipelineCache = nullptr;
        stagingBufferCache = nullptr;
        frameBufferCache = nullptr;
        layoutCache = nullptr;
        queues = nullptr;

        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwTerminate();
    }

    std::string VulkanDriver::GetDriverHeader() const
    {
        auto supportedMajor = VK_VERSION_MAJOR(apiVersion);
        auto supportedMinor = VK_VERSION_MINOR(apiVersion);
        return std::string(" - Vulkan ") +
            std::to_string(supportedMajor) +
            std::string(".") +
            std::to_string(supportedMinor);
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
            return physicalDeviceProperties.properties.limits.minStorageBufferOffsetAlignment;
        }

        if ((usage & BufferUsage::Constant) != 0)
        {
            return physicalDeviceProperties.properties.limits.minUniformBufferOffsetAlignment;
        }

        if ((usage & BufferUsage::ShaderBindingTable) != 0)
        {
            return physicalDeviceProperties.rayTracingProperties.shaderGroupBaseAlignment;
        }

        return sizeof(char);
    }

    void VulkanDriver::SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range)
    {
        globalResources.Set(nameHashId, Handle(buffer->GetNative<VulkanBuffer>()->GetBindHandle(range)));
    }

    void VulkanDriver::SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        globalResources.Set(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::SampledTexture)));
    }

    void VulkanDriver::SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray)
    {
        globalResources.Set(nameHashId, Handle(bufferArray->GetNative<VulkanBindArray>()));
    }

    void VulkanDriver::SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray)
    {
        globalResources.Set(nameHashId, Handle(textureArray->GetNative<VulkanBindArray>()));
    }

    void VulkanDriver::SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range)
    {
        globalResources.Set(nameHashId, Handle(texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::Image)));
    }

    void VulkanDriver::SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure)
    {
        globalResources.Set(nameHashId, Handle(structure->GetNative<VulkanAccelerationStructure>()->GetBindHandle()));
    }

    void VulkanDriver::SetConstant(uint32_t nameHashId, const void* data, uint32_t size)
    {
        globalResources.Set<char>(nameHashId, reinterpret_cast<const char*>(data), size);
    }

    void VulkanDriver::SetKeyword(uint32_t nameHashId, bool value)
    {
        globalResources.Set<bool>(nameHashId, value);
    }

    void VulkanDriver::GC()
    {
        stagingBufferCache->Prune();
        pipelineCache->Prune();
        descriptorCache->Prune();
        disposer->Prune();
        frameBufferCache->Prune();
        queues->Prune();
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


