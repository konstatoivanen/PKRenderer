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
        PK_RENDER_STATE_DIRTY_DESCRIPTORS = 1 << 5
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
        VulkanServiceContext& SetGlobalResources(PropertyBlock* value) { globalResources = value; return *this; }
        VulkanServiceContext& SetDescriptorCache(VulkanDescriptorCache* value) { descriptorCache = value; return *this; }
        VulkanServiceContext& SetPipelineCache(VulkanPipelineCache* value) { pipelineCache = value; return *this; }
        VulkanServiceContext& SetSamplerCache(VulkanSamplerCache* value) { samplerCache = value; return *this; }
        VulkanServiceContext& SetStagingBufferCache(VulkanStagingBufferCache* value) { stagingBufferCache = value; return *this; }
        VulkanServiceContext& SetBarrierHandler(VulkanBarrierHandler* value) { barrierHandler = value; return *this; }
        VulkanServiceContext& SetDisposer(Disposer* value) { disposer = value; return *this; }
    };

    struct VulkanVertexBufferBundle
    {
        VkBuffer buffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
        VkDeviceSize offsets[PK_RHI_MAX_VERTEX_ATTRIBUTES]{}; // Not used atm.
        uint32_t count = 0u;
    };

    struct VulkanDescriptorState
    {
        VulkanDescriptorCache::DescriptorBinding bindings[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        const VulkanDescriptorSet* descriptorSet = nullptr;
        VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        VkShaderStageFlagBits stageFlags = (VkShaderStageFlagBits)0;
        uint32_t bindingCount = 0u;
    };

    struct VulkanRenderTargetBindings
    {
        struct Attachment
        {
            VersionHandle<VulkanBindHandle> target;
            VersionHandle<VulkanBindHandle> resolve;
            VkResolveModeFlagBits resolveMode;
            LoadOp loadOp = LoadOp::Load;
            StoreOp storeOp = StoreOp::Store;
            TextureClearValue clearValue{};
        };

        VkRect2D area{};
        uint32_t layers = 0u;
        uint32_t colorCount = 0u;
        Attachment colors[PK_RHI_MAX_RENDER_TARGETS]{};
        Attachment depth{};
    };

    class VulkanRenderState : NoCopy
    {
        public:
            VulkanRenderState(const VulkanServiceContext& services) : m_services(services) {}

            constexpr bool HasPipeline() const { return m_pipeline != nullptr; }
            constexpr VulkanServiceContext* GetServices() { return &m_services; }
            constexpr VkPipeline GetPipeline() const { return m_pipeline->pipeline; }
            constexpr VkPipelineLayout GetPipelineLayout() const { return m_pipelineKey.shader->GetPipelineLayout()->layout; }
            constexpr VkShaderStageFlags GetPipelinePushConstantStageFlags() const { return m_pipelineKey.shader->GetPipelineLayout()->pushConstantStageFlags; }
            constexpr const ShaderPushConstantLayout& GetPipelinePushConstantLayout() const { return m_pipelineKey.shader->GetPushConstantLayout(); }
            constexpr VkDescriptorSet GetDescriptorSet() const { return m_descritorState.descriptorSet->set; }
            constexpr uint3 GetComputeGroupSize() const { return m_pipelineKey.shader->GetGroupSize(); }
            const char* GetShaderName() const { return m_pipelineKey.shader->GetName(); }
            inline VkPipelineBindPoint GetPipelineBindPoint() const { return VulkanEnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetStageFlags()); }
            VkRenderingInfo GetRenderPassInfo() const;
            VulkanVertexBufferBundle GetVertexBufferBundle() const;
            VkStridedDeviceAddressRegionKHR* GetShaderBindingTableAddresses();
            const VulkanBindHandle* GetIndexBuffer(VkIndexType* outIndexType) const;

            void Reset();
            void SetRenderTarget(const VulkanRenderTargetBindings& target);

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
            
            VkImageLayout RecordRenderTarget(const VulkanBindHandle* handle, 
                VkPipelineStageFlags stage, 
                VkAccessFlags access,
                VkImageLayout layout,
                uint8_t options);

            PKRenderStateDirtyFlags ValidatePipeline(const FenceRef& fence);

        private:
            void ValidateVertexBuffers();
            void ValidateResourceAccess();
            void ValidateDescriptors(const FenceRef& fence);
            void ValidatePipelineFormats();

            VulkanServiceContext m_services;
        
            VulkanDescriptorState m_descritorState{};
            VulkanPipelineCache::PipelineKey m_pipelineKey{};
            VulkanRenderTargetBindings m_renderTarget{};
            VkImageLayout m_renderTargetLayouts[PK_RHI_MAX_RENDER_TARGETS + 1u]{};
            VkStridedDeviceAddressRegionKHR m_sbtAddresses[(uint32_t)RayTracingShaderGroup::MaxCount]{};
        
            VertexStreamElement m_vertexStreamLayout[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_vertexBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_indexBuffer = nullptr;
            VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;
            
            VkViewport m_viewports[PK_RHI_MAX_VIEWPORTS]{};
            VkRect2D m_scissors[PK_RHI_MAX_VIEWPORTS]{};
            uint32_t m_dirtyFlags = 0u;

            const VulkanPipeline* m_pipeline = nullptr;
    };
}
