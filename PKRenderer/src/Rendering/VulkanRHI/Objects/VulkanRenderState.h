#pragma once
#include "Utilities/PropertyBlock.h"
#include "Rendering/VulkanRHI/Systems/VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanPipelineCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanSamplerCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanFrameBufferCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanStagingBufferCache.h"
#include "Rendering/VulkanRHI/Systems/VulkanDisposer.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI::Systems;

    enum PKRenderStateDirtyFlags
    {
        PK_RENDER_STATE_DIRTY_RENDERTARGET = 1 << 0,
        PK_RENDER_STATE_DIRTY_PIPELINE = 1 << 1,
        PK_RENDER_STATE_DIRTY_SHADER = 1 << 2,
        PK_RENDER_STATE_DIRTY_VERTEXBUFFERS = 1 << 3,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 = 1 << 4,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_1 = 1 << 5,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_2 = 1 << 6,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_3 = 1 << 7,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SETS = 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 | 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_1 | 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_2 | 
            PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_3,
    };

    struct VulkanSystemContext
    {
        VulkanDescriptorCache* descriptorCache;
        VulkanPipelineCache* pipelineCache;
        VulkanSamplerCache* samplerCache;
        VulkanFrameBufferCache* frameBufferCache;
        VulkanStagingBufferCache* stagingBufferCache;
        VulkanDisposer* disposer;
    };

    struct VulkanVertexBufferBundle
    {
        VkBuffer buffers[PK_MAX_VERTEX_ATTRIBUTES]{};
        VkDeviceSize offsets[PK_MAX_VERTEX_ATTRIBUTES]{}; // Not used atm.
        uint32_t count = 0u;
    };

    struct VulkanRenderState : NoCopy
    {
        VulkanRenderState(const VulkanSystemContext& systems) :
            m_descriptorCache(systems.descriptorCache),
            m_pipelineCache(systems.pipelineCache),
            m_samplerCache(systems.samplerCache),
            m_frameBufferCache(systems.frameBufferCache),
            m_stagingBufferCache(systems.stagingBufferCache),
            m_disposer(systems.disposer)
        {
        }

        void Reset();
        void SetRenderTarget(const VulkanRenderTarget* renderTargets, const VulkanRenderTarget* resolves, uint32_t count);
        void SetRenderArea(const VkRect2D& rect);
        void ClearColor(const color& color, uint32_t index);
        void ClearDepth(float depth, uint32_t stencil);
        void DiscardColor(uint32_t index);
        void DiscardDepth();

        void SetShader(const VulkanShader* shader);
        void SetBlending(const BlendParameters& blend);
        void SetRasterization(const RasterizationParameters& rasterization);
        void SetDepthStencil(const DepthStencilParameters& depthStencil);
        void SetMultisampling(const MultisamplingParameters& multisampling);
        void SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count);

        template<typename T>
        void SetResource(uint32_t nameHashId, const T* value, uint count = 1) { m_resourceProperties.Set(nameHashId, value, count); }

        template<typename T>
        void SetResource(uint32_t nameHashId, const T& value) { m_resourceProperties.Set(nameHashId, value); }

        VkRenderPassBeginInfo GetRenderPassInfo();
        VulkanVertexBufferBundle GetVertexBufferBundle();

        void ValidateRenderTarget();
        void ValidateVertexBuffers();
        void ValidateDescriptorSets(const VulkanExecutionGate& gate);
        PKRenderStateDirtyFlags ValidatePipeline(const VulkanExecutionGate& gate);

        PropertyBlock m_resourceProperties = PropertyBlock(16384);
        VulkanDescriptorCache* m_descriptorCache = nullptr;
        VulkanPipelineCache* m_pipelineCache = nullptr;
        VulkanSamplerCache* m_samplerCache = nullptr;
        VulkanFrameBufferCache* m_frameBufferCache = nullptr;
        VulkanStagingBufferCache* m_stagingBufferCache = nullptr;
        VulkanDisposer* m_disposer = nullptr;

        DescriptorSetKey m_descriptorSetKeys[PK_MAX_DESCRIPTOR_SETS]{};
        PipelineKey m_pipelineKey{};
        FrameBufferKey m_frameBufferKey[2]{};
        RenderPassKey m_renderPassKey[2]{};
        
        const VulkanBindHandle* m_vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES]{};

        VkClearValue m_clearValues[PK_MAX_RENDER_TARGETS + 1]{};
        uint32_t m_clearValueCount = 0u;
        VkRect2D m_renderArea{};
        
        uint32_t m_dirtyFlags;
        const VulkanRenderPass* m_renderPass = nullptr;
        const VulkanPipeline* m_pipeline = nullptr;
        const VulkanDescriptorSet* m_descriptorSets[PK_MAX_DESCRIPTOR_SETS];
        const VulkanFrameBuffer* m_frameBuffer = nullptr;
    };
}