#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/Services/Disposer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Services/VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Services/VulkanSamplerCache.h"
#include "Rendering/VulkanRHI/Services/VulkanPipelineCache.h"
#include "Rendering/VulkanRHI/Services/VulkanStagingBufferCache.h"
#include "Rendering/VulkanRHI/Services/VulkanCommandBufferPool.h"
#include "Rendering/VulkanRHI/Services/VulkanFrameBufferCache.h"
#include "Rendering/VulkanRHI/Services/VulkanLayoutCache.h"
#include "Rendering/VulkanRHI/Services/VulkanBarrierHandler.h"
#include "Rendering/VulkanRHI/Objects/VulkanQueue.h"

namespace PK::Rendering::VulkanRHI
{
    struct VulkanContextProperties
    {
        std::string appName;
        std::string workingDirectory;
        uint64_t garbagePruneDelay;
        uint32_t minApiVersionMajor;
        uint32_t minApiVersionMinor;
        const std::vector<const char*>* validationLayers;
        const std::vector<const char*>* contextualInstanceExtensions;
        const std::vector<const char*>* contextualDeviceExtensions;

        VulkanContextProperties(
            const std::string& appName = "Vulkan Engine",
            const std::string& workingDirectory = "",
            uint64_t garbagePruneDelay = 32ull,
            uint32_t minApiVersionMajor = 1,
            uint32_t minApiVersionMinor = 2,
            const std::vector<const char*>* validationLayers = nullptr,
            const std::vector<const char*>* contextualInstanceExtensions = nullptr,
            const std::vector<const char*>* contextualDeviceExtensions = nullptr) :
            appName(appName),
            workingDirectory(workingDirectory),
            garbagePruneDelay(garbagePruneDelay),
            minApiVersionMajor(minApiVersionMajor),
            minApiVersionMinor(minApiVersionMinor),
            validationLayers(validationLayers),
            contextualInstanceExtensions(contextualInstanceExtensions),
            contextualDeviceExtensions(contextualDeviceExtensions)
        {
        }
    };

    struct VulkanDriver : public GraphicsDriver
    {
        VulkanDriver(const VulkanContextProperties& properties);
        ~VulkanDriver();

        Rendering::Structs::APIType GetAPI() const override final { return Rendering::Structs::APIType::Vulkan; }
        Rendering::Objects::QueueSet* GetQueues() const override final { return queues.get(); }
        std::string GetDriverHeader() const;
        DriverMemoryInfo GetMemoryInfo() const override final;
        size_t GetBufferOffsetAlignment(Structs::BufferUsage usage) const override final;

        void SetBuffer(uint32_t nameHashId, Objects::Buffer* buffer, const Structs::IndexRange& range) override final;
        void SetTexture(uint32_t nameHashId, Objects::Texture* texture, const Structs::TextureViewRange& range) override final;
        void SetBufferArray(uint32_t nameHashId, Objects::BindArray<Objects::Buffer>* bufferArray) override final;
        void SetTextureArray(uint32_t nameHashId, Objects::BindArray<Objects::Texture>* textureArray) override final;
        void SetImage(uint32_t nameHashId, Objects::Texture* texture, const Structs::TextureViewRange& range) override final;
        void SetAccelerationStructure(uint32_t nameHashId, Objects::AccelerationStructure* structure) override final;
        void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) override final;
        void SetKeyword(uint32_t nameHashId, bool value) override final;

        void WaitForIdle() const override final { vkDeviceWaitIdle(device); }
        void GC() override final;

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VmaAllocator allocator;
        VulkanContextProperties properties;
        VulkanPhysicalDeviceProperties physicalDeviceProperties;
        uint32_t apiVersion;

        PK::Utilities::Scope<Objects::VulkanQueueSet> queues;
        PK::Utilities::Scope<Services::VulkanFrameBufferCache> frameBufferCache;
        PK::Utilities::Scope<Services::VulkanStagingBufferCache> stagingBufferCache;
        PK::Utilities::Scope<Services::VulkanDescriptorCache> descriptorCache;
        PK::Utilities::Scope<Services::VulkanPipelineCache> pipelineCache;
        PK::Utilities::Scope<Services::VulkanSamplerCache> samplerCache;
        PK::Utilities::Scope<Services::VulkanLayoutCache> layoutCache;
        PK::Utilities::Scope<Rendering::Services::Disposer> disposer;
    };
}