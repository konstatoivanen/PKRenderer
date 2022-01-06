#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/Structs/Enums.h"
#include "Math/FunctionsMisc.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Math;
    using namespace PK::Utilities;

    struct DescriptorSetLayoutKey
    {
        uint16_t counts[PK_MAX_DESCRIPTORS_PER_SET]{};
        VkDescriptorType types[PK_MAX_DESCRIPTORS_PER_SET]{};
        VkShaderStageFlags stageFlags = 0u;

        inline bool operator < (const DescriptorSetLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(DescriptorSetLayoutKey)) < 0;
        }
    };

    struct PipelineLayoutKey
    {
        VkDescriptorSetLayout setlayouts[PK_MAX_DESCRIPTOR_SETS]{};
        VkPushConstantRange pushConstants[(int)ShaderStage::MaxCount]{};

        inline bool operator < (const PipelineLayoutKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(PipelineLayoutKey)) < 0;
        }
    };

    class VulkanLayoutCache : public NoCopy
    {
        public:
            VulkanLayoutCache(VkDevice device) : m_device(device) {}

            VulkanDescriptorSetLayout* GetSetLayout(const DescriptorSetLayoutKey& key);
            VulkanPipelineLayout* GetPipelineLayout(const PipelineLayoutKey& key);

        private:
            VkDevice m_device;
            std::map<DescriptorSetLayoutKey, Scope<VulkanDescriptorSetLayout>> m_setlayouts;
            std::map<PipelineLayoutKey, Scope<VulkanPipelineLayout>> m_pipelineLayouts;
    };
}