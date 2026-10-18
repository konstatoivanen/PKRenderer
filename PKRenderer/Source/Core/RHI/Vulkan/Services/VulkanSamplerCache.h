#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Containers/Pool.h"
#include "Core/Base/Types/Ref.h"
#include "Core/Base/NoCopy.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanSamplerCache : public NoCopy
    {
        VulkanSamplerCache(VkDevice device) : m_device(device) {}
        ~VulkanSamplerCache();

        inline VkSampler GetSampler(const SamplerDescriptor& descriptor) { return GetPooledSampler(descriptor)->sampler; }
        inline VulkanResourceView GetResourceView(const SamplerDescriptor& descriptor) { return GetPooledSampler(descriptor)->view; }
        
    private:
        using SampelerHash = Hash::TMurmurHash<SamplerDescriptor>;

        struct Sampler
        {
            VkSampler sampler;
            VulkanResourceView view;
        };
        
        Sampler* GetPooledSampler(const SamplerDescriptor& descriptor);

        const VkDevice m_device;
        FixedMap<SamplerDescriptor, Sampler, PK_VK_MAX_SAMPLERS, SampelerHash, 2ull> m_samplers;
    };
}
