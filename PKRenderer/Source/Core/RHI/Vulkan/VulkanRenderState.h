#pragma once
#include "Core/Utilities/PropertyBlock.h"
#include "Core/Utilities/Disposer.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/Services/VulkanDescriptorCache.h"
#include "Core/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Core/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Core/RHI/Vulkan/Services/VulkanStagingBufferCache.h"
#include "Core/RHI/Vulkan/Services/VulkanBarrierHandler.h"

namespace PK
{
    enum PKRenderStateDirtyFlags
    {
        PK_RENDER_STATE_DIRTY_RENDERTARGET = 1 << 0,
        PK_RENDER_STATE_DIRTY_PIPELINE = 1 << 1,
        PK_RENDER_STATE_DIRTY_SHADER = 1 << 2,
        PK_RENDER_STATE_DIRTY_VERTEXBUFFERS = 1 << 3,
        PK_RENDER_STATE_DIRTY_INDEXBUFFER = 1 << 4,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 = 1 << 5,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_1 = 1 << 6,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_2 = 1 << 7,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_3 = 1 << 8,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SETS = 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 | 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_1 | 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_2 | 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_3,
    };

    struct VulkanServiceContext
    {
        PropertyBlock* globalResources = nullptr;
        VulkanDescriptorCache* descriptorCache = nullptr;
        VulkanPipelineCache* pipelineCache = nullptr;
        VulkanSamplerCache* samplerCache = nullptr;
        VulkanStagingBufferCache* stagingBufferCache = nullptr;
        VulkanBarrierHandler* barrierHandler = nullptr;
        Disposer* disposer = nullptr;
    };

    struct VulkanVertexBufferBundle
    {
        VkBuffer buffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
        VkDeviceSize offsets[PK_RHI_MAX_VERTEX_ATTRIBUTES]{}; // Not used atm.
        uint32_t count = 0u;
    };

    struct VulkanDescriptorSetBundle
    {
        VkDescriptorSet sets[PK_RHI_MAX_DESCRIPTOR_SETS]{};
        VkPipelineLayout layout;
        VkPipelineBindPoint bindPoint;
        uint32_t firstSet = 0u;
        uint32_t count = 0u;
    };


    struct VulkanRenderPassState
    {
        struct Attachment
        {
            VersionHandle<VulkanBindHandle> target;
            VersionHandle<VulkanBindHandle> resolve;
            VkImageLayout initialLayout;
            VkImageLayout finalLayout;
            VkResolveModeFlagBits resolveMode;
            LoadOp loadOp = LoadOp::Keep;
            StoreOp storeOp = StoreOp::Store;
            TextureClearValue clearValue{};
        };

        VkRenderingFlags flags = 0u;
        VkRect2D area{};
        uint32_t layers = 0u;
        uint32_t viewMask = 0u;
        uint32_t colorCount = 0u;
        bool dynamicTargets = false;
        Attachment colors[PK_RHI_MAX_RENDER_TARGETS]{};
        Attachment depth{};
    };

    class VulkanRenderState : NoCopy
    {
        public:
            VulkanRenderState(const VulkanServiceContext& services) : m_services(services) {}

            constexpr VulkanServiceContext* GetServices() { return &m_services; }
            constexpr const ShaderPushConstantLayout& GetPipelinePushConstantLayout() const { return m_pipelineKey.shader->GetPushConstantLayout(); }
            constexpr VkPipelineLayout GetPipelineLayout() const { return m_pipelineKey.shader->GetPipelineLayout()->layout; }
            constexpr VkPipeline GetPipeline() const { return m_pipeline->pipeline; }
            constexpr uint3 GetComputeGroupSize() const { return m_pipelineKey.shader->GetGroupSize(); }
            constexpr bool HasPipeline() const { return m_pipeline != nullptr; }
            constexpr bool HasDynamicTargets() const { return m_renderPass->dynamicTargets; }
            const char* GetShaderName() const { return m_pipelineKey.shader->GetName(); }
            inline VkPipelineBindPoint GetPipelineBindPoint() const { return VulkanEnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetStageFlags()); }
            VkRenderingInfo GetRenderPassInfo() const;
            VulkanVertexBufferBundle GetVertexBufferBundle() const;
            VulkanDescriptorSetBundle GetDescriptorSetBundle(const FenceRef& fence, uint32_t dirtyFlags);
            VkStridedDeviceAddressRegionKHR* GetShaderBindingTableAddresses();
            const VulkanBindHandle* GetIndexBuffer(VkIndexType* outIndexType) const;

            void Reset();
            void SetRenderTarget(const VulkanBindHandle* const* renderTargets, const VulkanBindHandle* const* resolves, uint32_t count);
            void ClearColor(const color& color, uint32_t index);
            void ClearDepth(float depth, uint32_t stencil);
            void DiscardColor(uint32_t index);
            void DiscardDepth();

            bool SetViewports(const uint4* rects, uint32_t& count, VkViewport** outViewports);
            bool SetScissors(const uint4* rects, uint32_t& count, VkRect2D** outScissors);

            void SetShader(const VulkanShader* shader);
            void SetStageExcludeMask(const ShaderStageFlags mask);
            void SetBlending(const BlendParameters& blend);
            void SetRasterization(const RasterizationParameters& rasterization);
            void SetDepthStencil(const DepthStencilParameters& depthStencil);
            void SetMultisampling(const MultisamplingParameters& multisampling);
            void SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count);
            void SetVertexStreams(const VertexStreamElement* elements, uint32_t count);
            void SetIndexBuffer(const VulkanBindHandle* handle, VkIndexType indexType);
            void SetShaderBindingTableAddress(RayTracingShaderGroup group, VkDeviceAddress address, size_t stride, size_t size);

            // AccessRecord Utilities
            void RecordBuffer(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access);
            
            void RecordImage(const VulkanBindHandle* handle, 
                VkPipelineStageFlags stage, 
                VkAccessFlags access, 
                VkImageLayout overrideLayout = VK_IMAGE_LAYOUT_MAX_ENUM, 
                uint8_t options = PK_RHI_ACCESS_OPT_BARRIER);
            
            VulkanBarrierHandler::AccessRecord RecordRenderTarget(const VulkanBindHandle* handle, 
                VkPipelineStageFlags stage, 
                VkAccessFlags access,
                VkImageLayout layout,
                uint8_t options);

            PKRenderStateDirtyFlags ValidatePipeline(const FenceRef& fence);

        private:
            void ValidateRenderTarget();
            void ValidateVertexBuffers();
            void ValidateDescriptorSets(const FenceRef& fence);

            void RecordResourceAccess();

            VulkanServiceContext m_services;
        
            VulkanDescriptorCache::SetKey m_descriptorSetKeys[PK_RHI_MAX_DESCRIPTOR_SETS]{};
            VulkanPipelineCache::PipelineKey m_pipelineKey{};
            VulkanRenderPassState m_renderPass[2]{};
            VkStridedDeviceAddressRegionKHR m_sbtAddresses[(uint32_t)RayTracingShaderGroup::MaxCount]{};
        
            VertexStreamElement m_vertexStreamLayout[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_vertexBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_indexBuffer = nullptr;
            VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;
            
            VkViewport m_viewports[PK_RHI_MAX_VIEWPORTS]{};
            VkRect2D m_scissors[PK_RHI_MAX_VIEWPORTS]{};
            uint32_t m_dirtyFlags;

            const VulkanPipeline* m_pipeline = nullptr;
            const VulkanDescriptorSet* m_descriptorSets[PK_RHI_MAX_DESCRIPTOR_SETS];
    };
}