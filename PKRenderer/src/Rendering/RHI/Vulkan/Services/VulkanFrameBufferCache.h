#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Utilities/HashHelpers.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FastMap.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    using namespace PK::Utilities;

    struct alignas(8) FrameBufferKey 
    {
        VkImageView color[PK_MAX_RENDER_TARGETS];
        VkImageView resolve[PK_MAX_RENDER_TARGETS];
        VkImageView depth;
        VkRenderPass renderPass;
        VkExtent2D extent;
        uint16_t layers;

        inline bool operator == (const FrameBufferKey& r) const noexcept 
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(FrameBufferKey)) == 0;
        }
    };

    struct alignas(8) AttachmentKey
    {
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;
        VkFormat format;
        LoadOp loadop;
        StoreOp storeop;
        bool resolve;
    };

    struct alignas(8) RenderPassKey 
    {
        AttachmentKey colors[PK_MAX_RENDER_TARGETS];
        AttachmentKey depth;
        VkAccessFlags accessMask;
        VkPipelineStageFlags stageMask;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

        // Allows the use of barriers inside a render pass. Intended for multiviewport shenanigans.
        bool dynamicTargets;

        inline bool operator == (const RenderPassKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(RenderPassKey)) == 0;
        }
    };

    class VulkanFrameBufferCache : public PK::Utilities::NoCopy
    {
        using FrameBufferKeyHash = PK::Utilities::HashHelpers::TMurmurHash<FrameBufferKey>;
        using RenderPassKeyHash = PK::Utilities::HashHelpers::TMurmurHash<RenderPassKey>;

        public:
            VulkanFrameBufferCache(VkDevice device, uint64_t pruneDelay);
            ~VulkanFrameBufferCache();

            struct FrameBufferValue
            {
                VulkanFrameBuffer* frameBuffer;
                uint64_t pruneTick;
            };

            struct RenderPassValue
            {
                VulkanRenderPass* renderPass;
                uint64_t pruneTick;
            };

            const VulkanFrameBuffer* GetFrameBuffer(const FrameBufferKey& key);
            const VulkanRenderPass* GetRenderPass(const RenderPassKey& key);
            void Prune();

        private:
            const VkDevice m_device;
            FixedPool<VulkanFrameBuffer, 512> m_frameBufferPool;
            FixedPool<VulkanRenderPass, 512> m_renderPassPool;
            FastMap<FrameBufferKey, FrameBufferValue, FrameBufferKeyHash> m_frameBuffers;
            FastMap<RenderPassKey, RenderPassValue, RenderPassKeyHash> m_renderPasses;
            std::unordered_map<VkRenderPass, uint32_t> m_renderPassReferenceCounts;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}