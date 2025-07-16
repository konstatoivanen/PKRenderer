#include "PrecompiledHeader.h"
#include "Core/Utilities/Parse.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanBuffer.h"
#include "Core/RHI/Vulkan/VulkanTexture.h"
#include "Core/RHI/Vulkan/VulkanAccelerationStructure.h"
#include "Core/RHI/Vulkan/VulkanBindArray.h"
#include "Core/RHI/Vulkan/VulkanSwapchain.h"
#include "Core/RHI/BuiltInResources.h"
#include "VulkanDriver.h"

#include "Core/Utilities/FastTypeIndex.h"

namespace PK
{
    VulkanDriver::VulkanDriver(const VulkanDriverDescriptor& properties) : properties(properties)
    {
        vkHandle = Platform::LoadLibrary(PK_VK_LIBRARY_NAME);

        PK_LOG_INFO("ID float: %u", pk_base_type_index<float>());
        PK_LOG_INFO("ID float: %u", pk_base_type_index<float&>());
        PK_LOG_INFO("ID float: %u", pk_base_type_index<float*>());
        PK_LOG_INFO("ID float: %u", pk_base_type_index<const float>());
        PK_LOG_INFO("ID float: %u", pk_base_type_index<const float&>());
        PK_LOG_INFO("ID float: %u", pk_base_type_index<const float*>());

        PK_LOG_INFO("ID float: %u", pk_base_type_index<float4>());
        PK_LOG_INFO("ID float: %u", pk_base_type_index<color>());

        VulkanAssertAPIVersion(properties.apiVersionMajor, properties.apiVersionMinor);

        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debugMessengerCreateInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugMessengerCreateInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugMessengerCreateInfo.pfnUserCallback = VulkanDebugCallback;
        debugMessengerCreateInfo.pUserData = nullptr;

        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = properties.appName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = properties.engineName.c_str();
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_API_VERSION(0, properties.apiVersionMajor, properties.apiVersionMinor, 0);
        apiVersion = appInfo.apiVersion;

        const char* VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";

        VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.pNext = &debugMessengerCreateInfo;
        instanceCreateInfo.enabledLayerCount = properties.enableValidation ? 1u : 0u;
        instanceCreateInfo.ppEnabledLayerNames = &VK_LAYER_KHRONOS_validation;

        auto instanceExtensions = *properties.contextualInstanceExtensions;
        instanceExtensions.push_back("VK_KHR_surface");
        instanceExtensions.push_back(PK_VK_SURFACE_EXTENSION_NAME);

        PK_THROW_ASSERT(VulkanValidateInstanceExtensions(&instanceExtensions), "Trying to enable unavailable extentions!");
        PK_THROW_ASSERT(VulkanValidateValidationLayers(instanceCreateInfo.ppEnabledLayerNames, instanceCreateInfo.enabledLayerCount), "Trying to enable unavailable validation layers!");

        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

        VK_ASSERT_RESULT_CTX(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Failed to create vulkan instance!");
        VulkanBindExtensionMethods(instance, properties.enableDebugNames);

        {
            PK_LOG_NEWLINE();
            PK_LOG_INFO_SCOPE("VulkanDriver.vkCreateInstance: with '%i' layers and '%i' extensions:", instanceCreateInfo.enabledLayerCount, instanceCreateInfo.enabledExtensionCount);

            for (auto i = 0u; i < instanceCreateInfo.enabledExtensionCount; ++i)
            {
                PK_LOG_INFO(instanceCreateInfo.ppEnabledExtensionNames[i]);
            }

            for (auto i = 0u; i < instanceCreateInfo.enabledLayerCount; ++i)
            {
                PK_LOG_INFO(instanceCreateInfo.ppEnabledLayerNames[i]);
            }

            PK_LOG_NEWLINE();
        }

        VK_ASSERT_RESULT_CTX(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger), "Failed to create debug messenger");

        VulkanPhysicalDeviceRequirements physicalDeviceRequirements{};
        physicalDeviceRequirements.versionMajor = properties.apiVersionMajor;
        physicalDeviceRequirements.versionMinor = properties.apiVersionMinor;
        physicalDeviceRequirements.features = properties.features;
        physicalDeviceRequirements.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        physicalDeviceRequirements.deviceExtensions = properties.contextualDeviceExtensions;

        // Create a temporary surface so that we can query & select a physical device with surface present capabilities.
        VkSurfaceKHR temporarySurface;
        VK_ASSERT_RESULT_CTX(VulkanCreateSurfaceKHR(instance, Platform::GetHelperWindow(), &temporarySurface), "Failed to create window surface!");

        VulkanSelectPhysicalDevice(instance, temporarySurface, physicalDeviceRequirements, &physicalDevice);
        physicalDeviceProperties = VulkanGetPhysicalDeviceProperties(physicalDevice);

        VulkanQueueSetInitializer queueInitializer(physicalDevice, temporarySurface);

        vkDestroySurfaceKHR(instance, temporarySurface, nullptr);

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

        {
            PK_LOG_INFO_SCOPE("VulkanDriver.vkCreateDevice: with '%i' extensions", createInfo.enabledExtensionCount);

            for (auto i = 0u; i < createInfo.enabledExtensionCount; ++i)
            {
                PK_LOG_INFO(createInfo.ppEnabledExtensionNames[i]);
            }

            PK_LOG_NEWLINE();
        }

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.vulkanApiVersion = physicalDeviceProperties.core.apiVersion;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        VK_ASSERT_RESULT_CTX(vmaCreateAllocator(&allocatorInfo, &allocator), "Failed to create a VMA allocator!");

        stagingBufferCache = CreateUnique<VulkanStagingBufferCache>(device, allocator, properties.gcPruneDelay);
        pipelineCache = CreateUnique<VulkanPipelineCache>(device, physicalDeviceProperties, properties.workingDirectory, properties.apiVersionMajor, properties.gcPruneDelay);
        samplerCache = CreateUnique<VulkanSamplerCache>(device);
        layoutCache = CreateUnique<VulkanLayoutCache>(device);
        disposer = CreateUnique<Disposer>();
        descriptorCache = CreateUnique<VulkanDescriptorCache>(device, 4, 100ull,
            std::initializer_list<std::pair<const VkDescriptorType, size_t>>({
                { VK_DESCRIPTOR_TYPE_SAMPLER, 100ull },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100ull },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100ull },
                { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10ull }
                }));

        queues = CreateUnique<VulkanQueueSet>(device,
            queueInitializer,
            VulkanServiceContext
            {
                &globalResources,
                descriptorCache.get(),
                pipelineCache.get(),
                samplerCache.get(),
                stagingBufferCache.get(),
                nullptr, // Assigned by queues
                disposer.get()
            });

        builtInResources = new BuiltInResources();
    }

    VulkanDriver::~VulkanDriver()
    {
        delete builtInResources;

        vkDeviceWaitIdle(device);

        descriptorCache = nullptr;
        disposer = nullptr;
        samplerCache = nullptr;
        pipelineCache = nullptr;
        stagingBufferCache = nullptr;
        layoutCache = nullptr;
        queues = nullptr;

        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);

        Platform::FreeLibrary(vkHandle);
    }

    FixedString32 VulkanDriver::GetDriverHeader() const
    {
        auto supportedMajor = VK_VERSION_MAJOR(apiVersion);
        auto supportedMinor = VK_VERSION_MINOR(apiVersion);
        return FixedString32(" - Vulkan %u.%u", supportedMajor, supportedMinor);
    }

    RHIDriverMemoryInfo VulkanDriver::GetMemoryInfo() const
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

        RHIDriverMemoryInfo info{};
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
            return physicalDeviceProperties.core.limits.minStorageBufferOffsetAlignment;
        }

        if ((usage & BufferUsage::Constant) != 0)
        {
            return physicalDeviceProperties.core.limits.minUniformBufferOffsetAlignment;
        }

        if ((usage & BufferUsage::ShaderBindingTable) != 0)
        {
            return physicalDeviceProperties.rayTracing.shaderGroupBaseAlignment;
        }

        return sizeof(char);
    }

    RHIBufferRef VulkanDriver::CreateBuffer(size_t size, BufferUsage usage, const char* name) { return CreateRef<VulkanBuffer>(size, usage, name); }
    RHITextureRef VulkanDriver::CreateTexture(const TextureDescriptor& descriptor, const char* name) { return CreateRef<VulkanTexture>(descriptor, name); }
    RHIAccelerationStructureRef VulkanDriver::CreateAccelerationStructure(const char* name) { return CreateRef<VulkanAccelerationStructure>(name); }
    RHITextureBindArrayRef VulkanDriver::CreateTextureBindArray(size_t capacity) { return CreateRef<VulkanBindArray>(capacity); }
    RHIBufferBindArrayRef VulkanDriver::CreateBufferBindArray(size_t capacity) { return CreateRef<VulkanBindArray>(capacity); }
    RHIShaderScope VulkanDriver::CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) { return CreateUnique<VulkanShader>(base, pVariant, name); }
    RHISwapchainScope VulkanDriver::CreateSwapchain(const SwapchainDescriptor& descriptor) { return CreateUnique<VulkanSwapchain>(this, descriptor); }

    RHIBuffer* VulkanDriver::AcquireStagingBuffer(size_t size) { return stagingBufferCache->Acquire(size, false, nullptr); }
    void VulkanDriver::ReleaseStagingBuffer(RHIBuffer* buffer, const FenceRef& fence) { stagingBufferCache->Release(buffer->GetNative<VulkanStagingBuffer>(), fence); }

    void VulkanDriver::SetBuffer(NameID name, RHIBuffer* buffer, const BufferIndexRange& range) { globalResources.Set(name, buffer->GetNative<VulkanBuffer>()->GetBindHandle(range)); }
    void VulkanDriver::SetTexture(NameID name, RHITexture* texture, const TextureViewRange& range) { globalResources.Set(name, texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::SampledTexture)); }
    void VulkanDriver::SetBufferArray(NameID name, RHIBindArray<RHIBuffer>* bufferArray) { globalResources.Set(name, bufferArray->GetNative<VulkanBindArray>()); }
    void VulkanDriver::SetTextureArray(NameID name, RHIBindArray<RHITexture>* textureArray) { globalResources.Set<const VulkanBindArray*>(name, textureArray->GetNative<VulkanBindArray>()); }
    void VulkanDriver::SetImage(NameID name, RHITexture* texture, const TextureViewRange& range) { globalResources.Set(name, texture->GetNative<VulkanTexture>()->GetBindHandle(range, TextureBindMode::Image)); }
    void VulkanDriver::SetSampler(NameID name, const SamplerDescriptor& sampler) { globalResources.Set(name, samplerCache->GetBindHandle(sampler)); }
    void VulkanDriver::SetAccelerationStructure(NameID name, RHIAccelerationStructure* structure) { globalResources.Set(name, structure->GetNative<VulkanAccelerationStructure>()->GetBindHandle()); }
    void VulkanDriver::SetConstant(NameID name, const void* data, uint32_t size) { globalResources.Set<char>(name, reinterpret_cast<const char*>(data), size); }
    void VulkanDriver::SetKeyword(NameID name, bool value) { globalResources.Set<bool>(name, value); }

    void VulkanDriver::GC()
    {
        stagingBufferCache->Prune();
        pipelineCache->Prune();
        descriptorCache->Prune();
        disposer->Prune();
        queues->Prune();
    }


    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDriver::VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData)
    {
        auto isValidationError = strstr(pCallbackData->pMessage, "Error") != nullptr;
        auto isLoaderMessage = strstr(pCallbackData->pMessageIdName, "Loader Message") != nullptr;
        auto isNsightBug = strstr(pCallbackData->pMessage, "VkPrivateDataSlotCreateInfo") != nullptr;
        messageSeverity = isValidationError ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : messageSeverity;
        messageSeverity = isLoaderMessage ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT : messageSeverity;
        messageSeverity = isNsightBug ? VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : messageSeverity;

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
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

        PK_LOG_RHI(pCallbackData->pMessage);
        return VK_FALSE;
    }
}
