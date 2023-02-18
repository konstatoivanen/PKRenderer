#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Math/FunctionsMisc.h"
#include "Utilities/FixedPool.h"
#include "Utilities/PointerMap.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Services
{
    struct alignas(8) DescriptorBinding
    {
        union
        {
            const VulkanBindHandle* handle;
            const VulkanBindHandle* const* handles;
        };

        Structs::ResourceType type;
        bool isArray;
        uint16_t count;
        uint64_t version;
    };

    struct DescriptorSetKey
    {
        VkShaderStageFlagBits stageFlags;
        DescriptorBinding bindings[Structs::PK_MAX_DESCRIPTORS_PER_SET];

        inline bool operator == (const DescriptorSetKey& other) const noexcept
        {
            return stageFlags == other.stageFlags && memcmp(this, &other, sizeof(DescriptorSetKey)) == 0;
        }
    };

    struct DescriptorSetKeyHash
    {
        std::size_t operator()(const DescriptorSetKey& k) const noexcept
        {
            constexpr uint64_t seed = 18446744073709551557;
            return PK::Utilities::HashHelpers::MurmurHash(&k, sizeof(DescriptorSetKey), seed);
        }
    };


    class VulkanDescriptorCache : public PK::Utilities::NoCopy
    {
        private:
            struct ExtinctPool
            {
                VulkanDescriptorPool* pool;
                mutable Structs::FenceRef fence;
                std::vector<uint32_t> extinctSetIndices;
            };

        public:
            VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);
            ~VulkanDescriptorCache();

            const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                        const DescriptorSetKey& key,
                                                        const Structs::FenceRef& fence);

            void Prune();

        private:
            void GrowPool(const Structs::FenceRef& fence);
            void GetDescriptorSets(VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const Structs::FenceRef& fence, bool throwOnFail);

            const std::map<VkDescriptorType, size_t> m_poolSizes;
            const VkDevice m_device;
            const size_t m_maxSets;
            const uint64_t m_pruneDelay;
            size_t m_sizeMultiplier = 0ull;
            uint64_t m_currentPruneTick = 0ull;
            
            VulkanDescriptorPool* m_currentPool = nullptr;
            PK::Utilities::FixedPool<VulkanDescriptorSet, 2048> m_setsPool;
            PK::Utilities::PointerMap<DescriptorSetKey, VulkanDescriptorSet, DescriptorSetKeyHash> m_sets;
            std::vector<ExtinctPool> m_extinctPools;
            std::vector<VkDescriptorImageInfo> m_writeImages;
            std::vector<VkDescriptorBufferInfo> m_writeBuffers;
            std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_writeAccerationStructures;
    };
}