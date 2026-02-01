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

        struct alignas(8) DescriptorBinding
        {
            union
            {
                const VulkanBindHandle* handle;
                const VulkanBindHandle* const* handles;
            };

            ShaderResourceType type;
            bool isArray;
            uint16_t count;
            uint64_t version;
        };

        struct SetKey
        {
            uint32_t poolIndex;
            VkShaderStageFlagBits stageFlags;
            DescriptorBinding bindings[PK_RHI_MAX_DESCRIPTORS_PER_SET];

            inline bool operator == (const SetKey& other) const noexcept
            {
                return stageFlags == other.stageFlags && memcmp(this, &other, sizeof(SetKey)) == 0;
            }
        };

        public:
            VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);

            const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, SetKey& key, const FenceRef& fence, const char* name);
            void Prune();

        private:
            using SetKeyHash = Hash::TMurmurHash<SetKey>;

            VkDescriptorPoolSize m_poolSizes[VK_DESCRIPTOR_TYPE_COUNT]{};
            VkDescriptorPoolCreateInfo m_poolCreateInfo;
            const VkDevice m_device;
            const uint64_t m_pruneDelay;
            uint64_t m_sizeMultiplier = 0ull;
            uint64_t m_currentPruneTick = 0ull;
            
            VulkanDescriptorPool* m_currentPool = nullptr;
            FixedPool<VulkanDescriptorSet, VK_MAX_DESCRIPTOR_SET_COUNT> m_setsPool;
            FixedPointerMap16<SetKey, VulkanDescriptorSet, VK_MAX_DESCRIPTOR_SET_COUNT, SetKeyHash> m_sets;
            FixedPool<VulkanDescriptorPool, 8> m_poolPool; // A great name for a great variable.
            FixedArena<8192ull> m_writeArena;
    };
}
