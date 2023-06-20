#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/Structs/Enums.h"
#include "Math/FunctionsMisc.h"

namespace PK::Rendering::VulkanRHI::Services
{
    struct DescriptorSetLayoutKey
    {
        uint16_t counts[Structs::PK_MAX_DESCRIPTORS_PER_SET]{};
        VkDescriptorType types[Structs::PK_MAX_DESCRIPTORS_PER_SET]{};
        VkShaderStageFlags stageFlags = 0u;

        inline bool operator == (const DescriptorSetLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(DescriptorSetLayoutKey)) == 0;
        }
    };

    struct PipelineLayoutKey
    {
        VkDescriptorSetLayout setlayouts[Structs::PK_MAX_DESCRIPTOR_SETS]{};
        VkPushConstantRange pushConstants[(int)Structs::ShaderStage::MaxCount]{};

        inline bool operator == (const PipelineLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineLayoutKey)) == 0;
        }
    };

    class VulkanLayoutCache : public PK::Utilities::NoCopy
    {
        using TDescriptorHash = PK::Utilities::HashHelpers::TMurmurHash<DescriptorSetLayoutKey>;
        using TPipelineHash = PK::Utilities::HashHelpers::TMurmurHash<PipelineLayoutKey>;

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