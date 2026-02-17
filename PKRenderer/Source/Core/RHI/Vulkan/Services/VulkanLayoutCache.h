#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/Ref.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
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
        VkDescriptorSetLayout setlayout = VK_NULL_HANDLE;
        VkPushConstantRange pushConstantRange{};

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
            const VulkanPipelineLayout* GetPipelineLayout(const PipelineLayoutKey& key, const char* name);
            void ReleaseSetLayout(const VulkanDescriptorSetLayout* layout, const FenceRef& releaseFence);
            void ReleasePipelineLayout(const VulkanPipelineLayout* layout, const FenceRef& releaseFence);
            void Prune();

        private:
            VkDevice m_device;
            FixedPool<VulkanDescriptorSetLayout, PK_VK_MAX_DESCRIPTOR_SET_LAYOUTS> m_setLayoutPool;
            FixedMap16<DescriptorSetLayoutKey, uint16_t, PK_VK_MAX_DESCRIPTOR_SET_LAYOUTS, TDescriptorHash> m_setLayoutMap;
            FixedPool<VulkanPipelineLayout, PK_VK_MAX_PIPELINE_LAYOUTS> m_pipelineLayoutPool;
            FixedMap16<PipelineLayoutKey, uint16_t, PK_VK_MAX_PIPELINE_LAYOUTS, TPipelineHash> m_pipelineLayoutMap;
    };
}
