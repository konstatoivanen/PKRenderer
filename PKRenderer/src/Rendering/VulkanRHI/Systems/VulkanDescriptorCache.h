#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Math/PKMath.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

    struct alignas(8) DescriptorBinding
    {
        const VulkanBindHandle* handle;
        ResourceType type;
        uint8_t binding;
        uint16_t count;
    };

    struct DescriptorSetKey
    {
        DescriptorBinding bindings[PK_MAX_DESCRIPTORS_PER_SET];

        inline bool operator == (const DescriptorSetKey& other) const noexcept
        {
            return memcmp(this, &other, sizeof(DescriptorSetKey)) == 0;
        }
    };

    struct DescriptorSetKeyHash
    {
        std::size_t operator()(const DescriptorSetKey& k) const noexcept
        {
            constexpr ulong seed = 18446744073709551557;
            return PK::Math::Functions::MurmurHash(&k, sizeof(DescriptorSetKey), seed);
        }
    };


    class VulkanDescriptorCache : public PK::Core::NoCopy
    {
        public:
            struct ExtinctPool
            {
                Ref<VulkanDescriptorPool> pool;
                mutable VulkanExecutionGate executionGate;
            };

            VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);

            const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                        const DescriptorSetKey& key,
                                                        const VulkanExecutionGate& gate);
            void Prune();

        private:
            void GrowPool(const VulkanExecutionGate& executionGate);
            void GetDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const VulkanExecutionGate& gate, bool throwOnFail);

            const VkDevice m_device;
            const std::map<VkDescriptorType, size_t> m_poolSizes;
            const size_t m_maxSets;
            size_t m_sizeMultiplier = 0ull;
            uint64_t m_currentPruneTick = 0ull;
            uint64_t m_pruneDelay;

            Ref<VulkanDescriptorPool> m_currentPool;
            std::unordered_map<DescriptorSetKey, VulkanDescriptorSet, DescriptorSetKeyHash> m_sets;
            std::vector<ExtinctPool> m_extinctPools;
    };
}