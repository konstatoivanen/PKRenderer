#pragma once
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/Ref.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDescriptorCache : public NoCopy
    {
        // Match at least minimum count of VkDescriptorType enum values.
        constexpr static uint32_t VK_DESCRIPTOR_TYPE_COUNT = 32u;

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
            VkShaderStageFlagBits stageFlags;
            DescriptorBinding bindings[PK_RHI_MAX_DESCRIPTORS_PER_SET];

            inline bool operator == (const SetKey& other) const noexcept
            {
                return stageFlags == other.stageFlags && memcmp(this, &other, sizeof(SetKey)) == 0;
            }
        };

        private:
            struct ExtinctPool
            {
                uint32_t poolIndex;
                mutable FenceRef fence;
                FixedMask<2048> indexMask;
            };

            using SetKeyHash = Hash::TMurmurHash<SetKey>;

        public:
            VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);
            const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                        const SetKey& key,
                                                        const FenceRef& fence,
                                                        const char* name);
            void Prune();

        private:
            void GrowPool(const FenceRef& fence);
            void GetDescriptorSets(VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const FenceRef& fence, bool throwOnFail);

            VkDescriptorPoolSize m_poolSizes[VK_DESCRIPTOR_TYPE_COUNT]{};
            VkDescriptorPoolCreateInfo m_poolCreateInfo;
            const VkDevice m_device;
            const uint64_t m_pruneDelay;
            uint64_t m_sizeMultiplier = 0ull;
            uint64_t m_currentPruneTick = 0ull;
            
            VulkanDescriptorPool* m_currentPool = nullptr;
            FixedPool<VulkanDescriptorSet, 2048> m_setsPool;
            FixedPool<VulkanDescriptorPool, 8> m_poolPool; // A great name for a great variable.
            FixedPointerMap16<SetKey, VulkanDescriptorSet, 2048u, SetKeyHash> m_sets;
            std::vector<ExtinctPool> m_extinctPools;
            std::vector<VkDescriptorImageInfo> m_writeImages;
            std::vector<VkDescriptorBufferInfo> m_writeBuffers;
            std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_writeAccerationStructures;
    };
}