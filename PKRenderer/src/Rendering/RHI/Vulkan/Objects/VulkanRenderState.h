#pragma once
#include "Utilities/PropertyBlock.h"
#include "Utilities/Disposer.h"
#include "Rendering/RHI/Vulkan/VulkanCommon.h"
#include "Rendering/RHI/Vulkan/Services/VulkanDescriptorCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanSamplerCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanFrameBufferCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanStagingBufferCache.h"
#include "Rendering/RHI/Vulkan/Services/VulkanBarrierHandler.h"

namespace PK::Rendering::RHI::Vulkan::Objects
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
        PK::Utilities::PropertyBlock* globalResources = nullptr;
        Services::VulkanDescriptorCache* descriptorCache = nullptr;
        Services::VulkanPipelineCache* pipelineCache = nullptr;
        Services::VulkanSamplerCache* samplerCache = nullptr;
        Services::VulkanFrameBufferCache* frameBufferCache = nullptr;
        Services::VulkanStagingBufferCache* stagingBufferCache = nullptr;
        Services::VulkanBarrierHandler* barrierHandler = nullptr;
        PK::Utilities::Disposer* disposer = nullptr;
    };

    struct VulkanVertexBufferBundle
    {
        VkBuffer buffers[PK_MAX_VERTEX_ATTRIBUTES]{};
        VkDeviceSize offsets[PK_MAX_VERTEX_ATTRIBUTES]{}; // Not used atm.
        uint32_t count = 0u;
    };

    struct VulkanDescriptorSetBundle
    {
        VkDescriptorSet sets[PK_MAX_DESCRIPTOR_SETS]{};
        VkPipelineLayout layout;
        VkPipelineBindPoint bindPoint;
        uint32_t firstSet = 0u;
        uint32_t count = 0u;
    };

    class VulkanRenderState : PK::Utilities::NoCopy
    {
        public:
            VulkanRenderState(const VulkanServiceContext& services) : m_services(services) {}

            constexpr VulkanServiceContext* GetServices() { return &m_services; }
            constexpr const PushConstantLayout& GetPipelinePushConstantLayout() const { return m_pipelineKey.shader->GetPushConstantLayout(); }
            constexpr VkPipelineLayout GetPipelineLayout() const { return m_pipelineKey.shader->GetPipelineLayout()->layout; }
            constexpr VkPipeline GetPipeline() const { return m_pipeline->pipeline; }
            constexpr Math::uint3 GetComputeGroupSize() const { return m_pipelineKey.shader->GetGroupSize(); }
            constexpr bool HasPipeline() const { return m_pipeline != nullptr; }
            constexpr bool HasDynamicTargets() const { return m_renderPassKey->dynamicTargets; }
            const char* GetShaderName() const { return m_pipelineKey.shader->GetName(); }
            inline VkPipelineBindPoint GetPipelineBindPoint() const { return EnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetStageFlags()); }
            VkRenderPassBeginInfo GetRenderPassInfo() const;
            VulkanVertexBufferBundle GetVertexBufferBundle() const;
            VulkanDescriptorSetBundle GetDescriptorSetBundle(const PK::Utilities::FenceRef& fence, uint32_t dirtyFlags);
            VkStridedDeviceAddressRegionKHR* GetShaderBindingTableAddresses();
            const VulkanBindHandle* GetIndexBuffer(VkIndexType* outIndexType) const;

            void Reset();
            void SetRenderTarget(const VulkanBindHandle* const* renderTargets, const VulkanBindHandle* const* resolves, uint32_t count);
            void ClearColor(const Math::color& color, uint32_t index);
            void ClearDepth(float depth, uint32_t stencil);
            void DiscardColor(uint32_t index);
            void DiscardDepth();

            bool SetViewports(const Math::uint4* rects, uint32_t& count, VkViewport** outViewports);
            bool SetScissors(const Math::uint4* rects, uint32_t& count, VkRect2D** outScissors);

            void SetShader(const Objects::VulkanShader* shader);
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
            void RecordImage(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout overrideLayout = VK_IMAGE_LAYOUT_MAX_ENUM, uint8_t options = Services::PK_ACCESS_OPT_BARRIER);
            Services::VulkanBarrierHandler::AccessRecord ExchangeImage(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access);

            PKRenderStateDirtyFlags ValidatePipeline(const PK::Utilities::FenceRef& fence);

        private:
            void ValidateRenderTarget();
            void ValidateVertexBuffers();
            void ValidateDescriptorSets(const PK::Utilities::FenceRef& fence);

            void RecordResourceAccess();
            void RecordRenderTargetAccess();

            VulkanServiceContext m_services;
        
            Services::DescriptorSetKey m_descriptorSetKeys[PK_MAX_DESCRIPTOR_SETS]{};
            Services::PipelineKey m_pipelineKey{};
            Services::FrameBufferKey m_frameBufferKey[2]{};
            Services::RenderPassKey m_renderPassKey[2]{};
            const VulkanBindHandle* m_frameBufferImages[PK_MAX_RENDER_TARGETS * 2 + 1]{};
            VkStridedDeviceAddressRegionKHR m_sbtAddresses[(uint32_t)RayTracingShaderGroup::MaxCount]{};
        
            VertexStreamElement m_vertexStreamLayout[PK_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_indexBuffer = nullptr;
            VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;
            
            VkViewport m_viewports[PK_MAX_VIEWPORTS]{};
            VkRect2D m_scissors[PK_MAX_VIEWPORTS]{};
            VkClearValue m_clearValues[PK_MAX_RENDER_TARGETS + 1]{};
            uint32_t m_clearValueCount = 0u;
            uint32_t m_dirtyFlags;

            const VulkanRenderPass* m_renderPass = nullptr;
            const VulkanPipeline* m_pipeline = nullptr;
            const VulkanDescriptorSet* m_descriptorSets[PK_MAX_DESCRIPTOR_SETS];
            const VulkanFrameBuffer* m_frameBuffer = nullptr;
    };
}