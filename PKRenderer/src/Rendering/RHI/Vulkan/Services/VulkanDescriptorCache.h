#pragma once
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"
#include "Math/FunctionsMisc.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    struct alignas(8) DescriptorBinding
    {
        union
        {
            const VulkanBindHandle* handle;
            const VulkanBindHandle* const* handles;
        };

        ResourceType type;
        bool isArray;
        uint16_t count;
        uint64_t version;
    };

    struct DescriptorSetKey
    {
        VkShaderStageFlagBits stageFlags;
        DescriptorBinding bindings[PK_MAX_DESCRIPTORS_PER_SET];

        inline bool operator == (const DescriptorSetKey& other) const noexcept
        {
            return stageFlags == other.stageFlags && memcmp(this, &other, sizeof(DescriptorSetKey)) == 0;
        }
    };

    class VulkanDescriptorCache : public PK::Utilities::NoCopy
    {
        private:
            struct ExtinctPool
            {
                uint32_t poolIndex;
                mutable FenceRef fence;
                PK::Utilities::Bitmask<2048> indexMask;
            };

            using DescriptorSetKeyHash = PK::Utilities::HashHelpers::TMurmurHash<DescriptorSetKey>;

        public:
            VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);
            const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                        const DescriptorSetKey& key, 
                                                        const FenceRef& fence,
                                                        const char* name);
            void Prune();

        private:
            void GrowPool(const FenceRef& fence);
            void GetDescriptorSets(VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const FenceRef& fence, bool throwOnFail);

            const std::map<VkDescriptorType, size_t> m_poolSizes;
            const VkDevice m_device;
            const size_t m_maxSets;
            const uint64_t m_pruneDelay;
            size_t m_sizeMultiplier = 1ull;
            uint64_t m_currentPruneTick = 0ull;
            
            VulkanDescriptorPool* m_currentPool = nullptr;
            PK::Utilities::FixedPool<VulkanDescriptorSet, 2048> m_setsPool;
            PK::Utilities::FixedPool<VulkanDescriptorPool, 8> m_poolPool; // A great name for a great variable.
            PK::Utilities::PointerMap<DescriptorSetKey, VulkanDescriptorSet, DescriptorSetKeyHash> m_sets;
            std::vector<ExtinctPool> m_extinctPools;
            std::vector<VkDescriptorImageInfo> m_writeImages;
            std::vector<VkDescriptorBufferInfo> m_writeBuffers;
            std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_writeAccerationStructures;
    };
}