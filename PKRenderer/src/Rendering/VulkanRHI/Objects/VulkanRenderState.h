#pragma once
#include "Utilities/PropertyBlock.h"
#include "Rendering/VulkanRHI/Services/VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Services/VulkanPipelineCache.h"
#include "Rendering/VulkanRHI/Services/VulkanSamplerCache.h"
#include "Rendering/VulkanRHI/Services/VulkanFrameBufferCache.h"
#include "Rendering/VulkanRHI/Services/VulkanStagingBufferCache.h"
#include "Rendering/VulkanRHI/Services/VulkanBarrierHandler.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/Services/Disposer.h"

namespace PK::Rendering::VulkanRHI::Objects
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
        Rendering::Services::Disposer* disposer = nullptr;
    };

    struct VulkanVertexBufferBundle
    {
        VkBuffer buffers[Structs::PK_MAX_VERTEX_ATTRIBUTES]{};
        VkDeviceSize offsets[Structs::PK_MAX_VERTEX_ATTRIBUTES]{}; // Not used atm.
        uint32_t count = 0u;
    };

    struct VulkanDescriptorSetBundle
    {
        VkDescriptorSet sets[Structs::PK_MAX_DESCRIPTOR_SETS]{};
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
            constexpr const Structs::ConstantBufferLayout& GetPipelinePushConstantLayout() const { return m_pipelineKey.shader->GetConstantLayout(); }
            constexpr VkPipelineLayout GetPipelineLayout() const { return m_pipelineKey.shader->GetPipelineLayout()->layout; }
            constexpr VkPipeline GetPipeline() const { return m_pipeline->pipeline; }
            constexpr Math::uint3 GetComputeGroupSize() const { return m_pipelineKey.shader->GetGroupSize(); }
            constexpr bool HasPipeline() const { return m_pipeline != nullptr; }
            constexpr bool HasDynamicTargets() const { return m_renderPassKey->dynamicTargets; }
            const std::string& GetShaderName() const { return m_pipelineKey.shader->GetName(); }
            inline VkPipelineBindPoint GetPipelineBindPoint() const { return EnumConvert::GetPipelineBindPoint(m_pipelineKey.shader->GetType()); }
            VkRenderPassBeginInfo GetRenderPassInfo() const;
            VulkanVertexBufferBundle GetVertexBufferBundle() const;
            VulkanDescriptorSetBundle GetDescriptorSetBundle(const Structs::FenceRef& fence, uint32_t dirtyFlags);
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
            void SetBlending(const Structs::BlendParameters& blend);
            void SetRasterization(const Structs::RasterizationParameters& rasterization);
            void SetDepthStencil(const Structs::DepthStencilParameters& depthStencil);
            void SetMultisampling(const Structs::MultisamplingParameters& multisampling);
            void SetVertexBuffers(const VulkanBindHandle** handles, uint32_t count);
            void SetIndexBuffer(const VulkanBindHandle* handle, VkIndexType indexType);
            void SetShaderBindingTableAddress(Structs::RayTracingShaderGroup group, VkDeviceAddress address, size_t stride, size_t size);

            // AccessRecord Utilities
            void RecordBuffer(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access);
            void RecordImage(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout overrideLayout = VK_IMAGE_LAYOUT_MAX_ENUM, uint8_t options = Services::PK_ACCESS_OPT_BARRIER);
            Services::VulkanBarrierHandler::AccessRecord ExchangeImage(const VulkanBindHandle* handle, VkPipelineStageFlags stage, VkAccessFlags access);

            PKRenderStateDirtyFlags ValidatePipeline(const Structs::FenceRef& fence);

        private:
            void ValidateRenderTarget();
            void ValidateVertexBuffers();
            void ValidateDescriptorSets(const Structs::FenceRef& fence);

            void RecordResourceAccess();
            void RecordRenderTargetAccess();

            VulkanServiceContext m_services;
        
            Services::DescriptorSetKey m_descriptorSetKeys[Structs::PK_MAX_DESCRIPTOR_SETS]{};
            Services::PipelineKey m_pipelineKey{};
            Services::FrameBufferKey m_frameBufferKey[2]{};
            Services::RenderPassKey m_renderPassKey[2]{};
            const VulkanBindHandle* m_frameBufferImages[Structs::PK_MAX_RENDER_TARGETS * 2 + 1]{};
            VkStridedDeviceAddressRegionKHR m_sbtAddresses[(uint32_t)Structs::RayTracingShaderGroup::MaxCount]{};
        
            const VulkanBindHandle* m_vertexBuffers[Structs::PK_MAX_VERTEX_ATTRIBUTES]{};
            const VulkanBindHandle* m_indexBuffer = nullptr;
            VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;
            
            VkViewport m_viewports[Structs::PK_MAX_VIEWPORTS]{};
            VkRect2D m_scissors[Structs::PK_MAX_VIEWPORTS]{};
            VkClearValue m_clearValues[Structs::PK_MAX_RENDER_TARGETS + 1]{};
            uint32_t m_clearValueCount = 0u;
            uint32_t m_dirtyFlags;

            const VulkanRenderPass* m_renderPass = nullptr;
            const VulkanPipeline* m_pipeline = nullptr;
            const VulkanDescriptorSet* m_descriptorSets[Structs::PK_MAX_DESCRIPTOR_SETS];
            const VulkanFrameBuffer* m_frameBuffer = nullptr;
    };
}