#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Utilities/VulkanConstants.h"
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"
#include "Math/PKMath.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace PK::Utilities;
    using namespace PK::Math;
    
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
        VkImageLayout layout;
        VkFormat format;
        LoadOp loadop;
        StoreOp storeop;
        bool resolve;
    };

    struct alignas(8) RenderPassKey 
    {
        AttachmentKey colors[PK_MAX_RENDER_TARGETS];
        AttachmentKey depth;
        uint32_t samples = 1;

        inline bool operator == (const RenderPassKey& r) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&r), sizeof(RenderPassKey)) == 0;
        }
    };

    struct FrameBufferKeyHash
    {
        std::size_t operator()(const FrameBufferKey& k) const noexcept
        {
            constexpr ulong seed = 18446744073709551557;
            return PK::Math::Functions::MurmurHash(reinterpret_cast<const void*>(&k), sizeof(FrameBufferKey), seed);
        }
    };

    struct RenderPassKeyHash
    {
        std::size_t operator()(const RenderPassKey& k) const noexcept
        {
            constexpr ulong seed = 18446744073709551557;
            return PK::Math::Functions::MurmurHash(reinterpret_cast<const void*>(&k), sizeof(RenderPassKey), seed);
        }
    };

    class VulkanFrameBufferCache : public PK::Core::NoCopy
    {
        public:
            VulkanFrameBufferCache(VkDevice device, uint64_t pruneDelay);

            struct FrameBufferValue
            {
                Ref<VulkanFrameBuffer> frameBuffer;
                uint64_t pruneTick;
            };

            struct RenderPassValue
            {
                Ref<VulkanRenderPass> renderPass;
                uint64_t pruneTick;
            };

            const VulkanFrameBuffer* GetFrameBuffer(const FrameBufferKey& key);
            const VulkanRenderPass* GetRenderPass(const RenderPassKey& key);
            void Prune();

        private:
            const VkDevice m_device;
            std::unordered_map<FrameBufferKey, FrameBufferValue, FrameBufferKeyHash> m_framebuffers;
            std::unordered_map<RenderPassKey, RenderPassValue, RenderPassKeyHash> m_renderPasses;
            std::unordered_map<VkRenderPass, uint32_t> m_renderPassReferenceCounts;
            uint64_t m_currentPruneTick = 0;
            uint64_t m_pruneDelay = 0;
    };
}