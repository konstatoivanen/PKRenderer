#pragma once
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Math/FunctionsMisc.h"
#include "Utilities/Pool.h"
#include "Utilities/PointerMap.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;

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

    struct DescriptorSetKeyHash
    {
        std::size_t operator()(const DescriptorSetKey& k) const noexcept
        {
            constexpr ulong seed = 18446744073709551557;
            return HashHelpers::MurmurHash(&k, sizeof(DescriptorSetKey), seed);
        }
    };


    class VulkanDescriptorCache : public NoCopy
    {
        private:
            struct ExtinctPool
            {
                VulkanDescriptorPool* pool;
                mutable ExecutionGate executionGate;
                std::vector<uint32_t> extinctSetIndices;
            };

        public:
            VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes);
            ~VulkanDescriptorCache();

            const VulkanDescriptorSet* GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                        const DescriptorSetKey& key,
                                                        const ExecutionGate& gate);

            void Prune();

        private:
            void GrowPool(const ExecutionGate& executionGate);
            void GetDescriptorSets(VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const ExecutionGate& gate, bool throwOnFail);

            const std::map<VkDescriptorType, size_t> m_poolSizes;
            const VkDevice m_device;
            const size_t m_maxSets;
            const uint64_t m_pruneDelay;
            size_t m_sizeMultiplier = 0ull;
            uint64_t m_currentPruneTick = 0ull;
            
            VulkanDescriptorPool* m_currentPool = nullptr;
           // Pool<VulkanBindHandle, 4096> m_bindHandlePool;
            Pool<VulkanDescriptorSet, 2048> m_setsPool;
            PointerMap<DescriptorSetKey, VulkanDescriptorSet, DescriptorSetKeyHash> m_sets;
            std::vector<ExtinctPool> m_extinctPools;
            std::vector<VkDescriptorImageInfo> m_writeImages;
            std::vector<VkDescriptorBufferInfo> m_writeBuffers;
    };
}