#pragma once
#include "Math/FunctionsMisc.h"
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Vulkan/VulkanCommon.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    struct DescriptorSetLayoutKey
    {
        uint16_t counts[PK_MAX_DESCRIPTORS_PER_SET]{};
        VkDescriptorType types[PK_MAX_DESCRIPTORS_PER_SET]{};
        VkShaderStageFlags stageFlags = 0u;

        inline bool operator == (const DescriptorSetLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(DescriptorSetLayoutKey)) == 0;
        }
    };

    struct PipelineLayoutKey
    {
        VkDescriptorSetLayout setlayouts[PK_MAX_DESCRIPTOR_SETS]{};
        VkPushConstantRange pushConstants[(int)ShaderStage::MaxCount]{};

        inline bool operator == (const PipelineLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineLayoutKey)) == 0;
        }
    };

    class VulkanLayoutCache : public PK::Utilities::NoCopy
    {
        using TDescriptorHash = PK::Utilities::Hash::TMurmurHash<DescriptorSetLayoutKey>;
        using TPipelineHash = PK::Utilities::Hash::TMurmurHash<PipelineLayoutKey>;

        public:
            VulkanLayoutCache(VkDevice device) : m_device(device) {}

            const VulkanDescriptorSetLayout* GetSetLayout(const DescriptorSetLayoutKey& key);
            const VulkanPipelineLayout* GetPipelineLayout(const PipelineLayoutKey& key);

        private:
            VkDevice m_device;
            PK::Utilities::FixedPool<VulkanDescriptorSetLayout, 1024> m_setLayoutPool;
            PK::Utilities::FixedPool<VulkanPipelineLayout, 1024> m_pipelineLayoutPool;
            PK::Utilities::PointerMap<DescriptorSetLayoutKey, VulkanDescriptorSetLayout, TDescriptorHash> m_setlayouts;
            PK::Utilities::PointerMap<PipelineLayoutKey, VulkanPipelineLayout, TPipelineHash> m_pipelineLayouts;
    };
}