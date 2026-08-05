#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/Pool.h"
#include "Core/Base/Types/Ref.h"
#include "Core/Base/NoCopy.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanSamplerCache : NoCopy
    {
        using SampelerHash = Hash::TMurmurHash<SamplerDescriptor>;

        VulkanSamplerCache(VkDevice device) : m_device(device) {}
        VkSampler GetSampler(const SamplerDescriptor& descriptor);
        const VulkanBindHandle* GetBindHandle(const SamplerDescriptor& descriptor);
        
    private:
        VulkanSampler* GetPooledSampler(const SamplerDescriptor& descriptor);

        const VkDevice m_device;
        FixedPool<VulkanBindHandle, PK_VK_MAX_SAMPLERS> m_bindhandlePool;
        FixedPool<VulkanSampler, PK_VK_MAX_SAMPLERS> m_samplerPool;
        FixedMap<SamplerDescriptor, VulkanSampler*, PK_VK_MAX_SAMPLERS, SampelerHash, 2ull> m_samplers;
    };
}
