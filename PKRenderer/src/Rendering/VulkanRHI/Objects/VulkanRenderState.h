#pragma once
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
        PK_RENDER_STATE_DIRTY_PIPELINE = 1 << 0,
        PK_RENDER_STATE_DIRTY_VERTEXBUFFERS = 1 << 1,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 = 1 << 2,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_1 = 1 << 3,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_2 = 1 << 4,
        PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_3 = 1 << 5,
    };

    struct VulkanRenderState
    {
        VulkanRenderState() {}
        VulkanRenderState(VulkanDescriptorCache* descriptorCache,
                          VulkanPipelineCache* pipelineCache,
                          VulkanSamplerCache* samplerCache,
                          VulkanFrameBufferCache* frameBufferCache,
                          VulkanStagingBufferCache* stagingBufferCache,
                          VulkanDisposer* disposer) :
            descriptorCache(descriptorCache),
            pipelineCache(pipelineCache),
            samplerCache(samplerCache),
            frameBufferCache(frameBufferCache),
            stagingBufferCache(stagingBufferCache),
            disposer(disposer)
        {
        }

        void Reset();
        void PrepareRenderPass();
        void SetRenderTarget(const VulkanRenderTarget& renderTarget, uint32_t index);
        void SetResolveTarget(const VulkanRenderTarget& renderTarget, uint32_t index);
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
        void SetResource(uint32_t nameHashId, const VulkanBindHandle* handle);
        void SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count);

        const VulkanPipeline* GetComputePipeline(const VulkanShader* shader);
        PKRenderStateDirtyFlags ValidatePipeline(const VulkanExecutionGate& gate);

        VulkanDescriptorCache* descriptorCache = nullptr;
        VulkanPipelineCache* pipelineCache = nullptr;
        VulkanSamplerCache* samplerCache = nullptr;
        VulkanFrameBufferCache* frameBufferCache = nullptr;
        VulkanStagingBufferCache* stagingBufferCache = nullptr;
        VulkanDisposer* disposer = nullptr;

        DescriptorSetKey descriptorSetKeys[PK_MAX_DESCRIPTOR_SETS]{};
        PipelineKey pipelineKey{};
        FrameBufferKey frameBufferKey{};
        RenderPassKey renderPassKey{};
        
        const VulkanBindHandle* vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES]{};
        VkBuffer vertexBuffersRaw[PK_MAX_VERTEX_ATTRIBUTES]{};
        VkDeviceSize vertexBufferOffsets[PK_MAX_VERTEX_ATTRIBUTES]{}; // Not used atm.
        uint32_t vertexBufferBindCount = 0u;

        VkClearValue clearValues[PK_MAX_RENDER_TARGETS + 1]{};
        uint32_t clearValueCount = 0u;
        VkRect2D renderArea{};
        
        bool pipelineIsDirty;
        bool vertexBuffersDirty;
        bool descriptorsDirty[PK_MAX_DESCRIPTOR_SETS];

        const VulkanRenderPass* renderPass = nullptr;
        const VulkanPipeline* pipeline = nullptr;
        const VulkanDescriptorSet* descriptorSets[PK_MAX_DESCRIPTOR_SETS];
        const VulkanFrameBuffer* frameBuffer = nullptr;
    };
}