#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/Pool.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanShader.h"

namespace PK
{
    struct VulkanDescriptorHeap : public NoCopy
    {
        using SamplerData = TLiteral<uint8_t, PK_VK_MAX_SAMPLER_DESCRIPTOR_SIZE>;
        using SamplerHash = Hash::TMurmurHash<SamplerDescriptor>;

        struct DescriptorHeap
        {
            VkBindHeapInfoEXT bindInfo{ VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT };
            VmaAllocation memory = nullptr;
            VkBuffer buffer = VK_NULL_HANDLE;
            uint8_t* mappedPtr = nullptr;
            VkDeviceSize arenaOffset = 0ull;
            VkDeviceSize arenaCapacity = 0ull;

            VkDeviceSize arenaHead = 0ull;
            VkDeviceSize arenaTail = 0ull;
            VkDeviceSize ranges[PK_VK_MAX_DESCRIPTOR_RANGES]{};
        };

        struct ResourceView
        {
            uint64_t index = 0ull;
            uint64_t pruneTick = 0ull;
        };

        struct ResourceData
        {
            uint3 resolution = PK_UINT3_ZERO;
            uint8_t data[PK_VK_MAX_RESOURCE_DESCRIPTOR_SIZE]{};
        };

        struct RenderTargetView
        {
            VkImageView view = VK_NULL_HANDLE;
            uint64_t pruneTick = 0ull;
        };

        VulkanDescriptorHeap(VkDevice device, 
            VmaAllocator allocator,
            const VulkanQueueFamilies& queueFamilies,
            const VulkanPhysicalDeviceProperties& properties,
            size_t resourceArenaSize,
            uint64_t pruneDelay);

        ~VulkanDescriptorHeap();

        constexpr const VkBindHeapInfoEXT& GetResourceHeapInfo() const { return m_resourceHeap.bindInfo; }
        constexpr const VkBindHeapInfoEXT& GetSamplerHeapInfo() const { return m_samplerHeap.bindInfo; }

        void BeginRange();
        uint64_t EndRange();
        void FreeRange(uint64_t rangeIndex);
        void Prune();

        VkImageView GetRenderTargetView(const VulkanResourceView& view);
        uint16_t GetSamplerIndex(const SamplerDescriptor& descriptor);
        uint32_t GetResourceIndex(const VulkanResourceView& view, ShaderResourceType type);
        uint3 GetTextureSize(uint32_t bindlessIndex) const;

        uint32_t AllocateDescriptorSet(
            const VulkanShader* shader,
            const VulkanDescriptorBinding* bindings,
            uint32_t count);

    private:
        static uint32_t AllocateDescriptorRange(DescriptorHeap& heap, uint32_t size, uint32_t alignment);

        const VkDevice m_device;
        const VmaAllocator m_allocator;
        const VulkanDescriptorHeapProperties m_properties;
        const uint64_t m_pruneDelay;
        uint64_t m_currentPruneTick = 0ull;

        uint64_t m_rangeCounter = 0ull;
        DescriptorHeap m_resourceHeap{};
        DescriptorHeap m_samplerHeap{};

        FixedMap<uint64_t, RenderTargetView, PK_VK_MAX_RENDER_TARGET_VIEWS> m_renderTargetViews;
        FixedMap<SamplerDescriptor, SamplerData, PK_VK_MAX_SAMPLERS, SamplerHash> m_samplerCache;

        FixedMap<uint64_t, ResourceView, PK_VK_MAX_DESCRIPTORS> m_resourceCache;
        FixedPool<ResourceData, PK_VK_MAX_DESCRIPTORS> m_resourcePool;
    };
}
