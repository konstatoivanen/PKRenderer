#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/Pool.h"
#include "Core/Base/Types/Ref.h"
#include "Core/Base/NoCopy.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct DescriptorSetLayoutKey
    {
        ShaderResourceType types[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        uint16_t counts[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        
        inline bool operator == (const DescriptorSetLayoutKey& r) const noexcept { return memcmp(this, &r, sizeof(DescriptorSetLayoutKey)) == 0; }
    };

    struct VulkanLayoutCache : public NoCopy
    {
        using TDescriptorHash = Hash::TMurmurHash<DescriptorSetLayoutKey>;

        VulkanLayoutCache(VkDevice device, const VulkanPhysicalDeviceProperties& deviceProperties) : 
            m_device(device), 
            m_maxPushConstantSize(deviceProperties.core.limits.maxPushConstantsSize),
            m_setLayoutMap(128ull, 3ull) {}

        const VulkanDescriptorSetLayout* GetSetLayout(const DescriptorSetLayoutKey& key);
        void ReleaseSetLayout(const VulkanDescriptorSetLayout* layout, const FenceRef& releaseFence);
        void Prune();

    private:
        const VkDevice m_device;
        const uint32_t m_maxPushConstantSize;
        FixedPool<VulkanDescriptorSetLayout, PK_VK_MAX_DESCRIPTOR_SET_LAYOUTS> m_setLayoutPool;
        FixedMap16<DescriptorSetLayoutKey, uint16_t, PK_VK_MAX_DESCRIPTOR_SET_LAYOUTS, TDescriptorHash> m_setLayoutMap;
    };
}
