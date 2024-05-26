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
        uint16_t counts[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        VkDescriptorType types[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        VkShaderStageFlags stageFlags = 0u;

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
            VulkanLayoutCache(VkDevice device) : m_device(device) {}

            const VulkanDescriptorSetLayout* GetSetLayout(const DescriptorSetLayoutKey& key);
            const VulkanPipelineLayout* GetPipelineLayout(const PipelineLayoutKey& key);

        private:
            VkDevice m_device;
            FixedPool<VulkanDescriptorSetLayout, 1024> m_setLayoutPool;
            FixedPool<VulkanPipelineLayout, 1024> m_pipelineLayoutPool;
            PointerMap<DescriptorSetLayoutKey, VulkanDescriptorSetLayout, TDescriptorHash> m_setlayouts = 
                PointerMap<DescriptorSetLayoutKey, VulkanDescriptorSetLayout, TDescriptorHash>(128ull);
            PointerMap<PipelineLayoutKey, VulkanPipelineLayout, TPipelineHash> m_pipelineLayouts = 
                PointerMap<PipelineLayoutKey, VulkanPipelineLayout, TPipelineHash>(128ull);
    };
}