#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct DescriptorSetLayoutKey
    {
        VkShaderStageFlags stageFlags = 0u;
        ShaderResourceType types[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        uint16_t counts[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};

        inline bool operator == (const DescriptorSetLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(DescriptorSetLayoutKey)) == 0;
        }
    };

    struct PipelineLayoutKey
    {
        VkDescriptorSetLayout setlayouts[PK_RHI_MAX_DESCRIPTOR_SETS]{};
        VkPushConstantRange pushConstants[PK_RHI_MAX_PUSH_CONSTANTS]{};

        inline bool operator == (const PipelineLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineLayoutKey)) == 0;
        }
    };

    class VulkanLayoutCache : public NoCopy
    {
        using TDescriptorHash = Hash::TMurmurHash<DescriptorSetLayoutKey>;
        using TPipelineHash = Hash::TMurmurHash<PipelineLayoutKey>;

        public:
            VulkanLayoutCache(VkDevice device) : m_device(device), m_setLayoutMap(128ull, 3ull), m_pipelineLayoutMap(128ull, 3ull) {}

            const VulkanDescriptorSetLayout* GetSetLayout(const DescriptorSetLayoutKey& key);
            const VulkanPipelineLayout* GetPipelineLayout(const PipelineLayoutKey& key);
            void ReleaseSetLayout(const VulkanDescriptorSetLayout* layout);
            void ReleasePipelineLayout(const VulkanPipelineLayout* layout);

        private:
            VkDevice m_device;
            FixedPool<VulkanDescriptorSetLayout, 1024> m_setLayoutPool;
            FixedPool<VulkanPipelineLayout, 1024> m_pipelineLayoutPool;
            FixedMap16<DescriptorSetLayoutKey, uint16_t, 1024u, TDescriptorHash> m_setLayoutMap;
            FixedMap16<PipelineLayoutKey, uint16_t, 1024u, TPipelineHash> m_pipelineLayoutMap;
    };
}
