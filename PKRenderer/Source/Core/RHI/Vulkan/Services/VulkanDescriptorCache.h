#pragma once
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/FixedArena.h"
#include "Core/Utilities/Ref.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDescriptorCache : public NoCopy
    {
        // Match at least minimum count of VkDescriptorType enum values.
        constexpr static uint32_t VK_DESCRIPTOR_TYPE_COUNT = 32u;
        constexpr static uint32_t VK_MAX_DESCRIPTOR_SET_COUNT = 1024u;
        constexpr static uint32_t VK_MAX_DESCRIPTOR_BINDINGS = 4096u;

        struct alignas(8) DescriptorBinding
        {
            union
            {
                const VulkanBindHandle* handle;
                const VulkanBindHandle* const* handles;
            };

            uint32_t version;
            uint16_t count;
            ShaderResourceType type;
            bool isArray;
        };

        VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);

        const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
            const DescriptorBinding* bindings,
            const uint32_t bindingCount,
            const FenceRef& fence, 
            const char* name);

        void Prune();

     private:
         VulkanDescriptorSet* AllocateDescriptorSet(VkDescriptorSetLayout layout, const uint32_t variableSize, const FenceRef& fence, const char* name);

        struct SetKey
        {
            const DescriptorBinding* bindings = nullptr;
            uint16_t count = 0u;
            uint16_t poolIndex = 0u;
            VkShaderStageFlagBits stageFlags = (VkShaderStageFlagBits)0u;
            bool operator == (const SetKey& other) const noexcept;
        };

        struct SetKeyHash
        {
            std::size_t operator()(const SetKey& k) const noexcept;
        };

        VkDescriptorPoolSize m_poolSizes[VK_DESCRIPTOR_TYPE_COUNT]{};
        VkDescriptorPoolCreateInfo m_poolCreateInfo;
        const VkDevice m_device;
        const uint64_t m_pruneDelay;
        uint64_t m_sizeMultiplier = 0ull;
        uint64_t m_currentPruneTick = 0ull;
        
        VulkanDescriptorPool* m_currentPool = nullptr;
        FixedPool<VulkanDescriptorSet, VK_MAX_DESCRIPTOR_SET_COUNT> m_setsPool;
        FixedPool<DescriptorBinding, VK_MAX_DESCRIPTOR_BINDINGS> m_bindingPool;
        FixedPointerMap16<SetKey, VulkanDescriptorSet, VK_MAX_DESCRIPTOR_SET_COUNT, SetKeyHash> m_sets;
        FixedPool<VulkanDescriptorPool, 8> m_poolPool; // A great name for a great variable.
        FixedArena<8192ull> m_writeArena;
    };
}
