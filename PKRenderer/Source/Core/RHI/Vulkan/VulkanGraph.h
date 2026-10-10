#pragma once
#include "Core/Base/Containers/PropertyBlock.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanStagingArena.h"
#include "Core/RHI/Vulkan/VulkanDescriptorArena.h"
#include "Core/RHI/Vulkan/VulkanTimerArena.h"
#include "Core/RHI/Vulkan/Services/VulkanPipelineCache.h"
#include "Core/RHI/RHIGraph.h"

namespace PK
{
    struct VulkanDriver;
    struct VulkanResourceState;
    struct VulkanSwapchain;

    struct VulkanGraphScope
    {
        FenceRef fence;
        uint64_t stagingRangeIndex = 0ull;
        uint64_t descriptorRangeIndex = 0ull;
        uint64_t timerTimelineIndex = 0ull;
        uint64_t invocationIndex = 0ull;
    };

    struct VulkanCmdContext
    {
        VkCommandBuffer cmb;
        VulkanSwapchain* swapchain;
        VkSemaphore acquireSignal;
        VkSemaphore presentSignal;
    };

    struct VulkanImageTransfer
    {
        VkImageMemoryBarrier2 barrier;
        VulkanImageTransfer* next = nullptr;
    };

    struct VulkanTransferBatch
    {
        VulkanImageTransfer* head = nullptr;
        uint32_t count = 0u;
        uint32_t srcQueueIndex = 0u;
        uint32_t dstQueueIndex = 0u;
        VkPipelineStageFlags2 srcStageMask = 0;
        VkPipelineStageFlags2 dstStageMask = 0;
    };

    struct VulkanCmd : public RHICmd
    {
        VkPipelineStageFlags2 stageFlags;
        uint64_t dirtyFlags;
        uint64_t dirtyFlagMask;
        uint64_t passIndex;

        // graph executor values
        VulkanTransferBatch* transfers = nullptr;
        const VkImageMemoryBarrier2* imageBarriers = nullptr;
        const VkBufferMemoryBarrier2* bufferBarriers = nullptr;
        uint32_t imageBarrierCount = 0u;
        uint32_t bufferBarrierCount = 0u;
    };

    struct VulkanShaderCmd : public VulkanCmd
    {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkPipelineBindPoint bindpoint;
        VkDeviceSize descriptorSetOffset;
        void* pushConstants;
        uint32_t pushConstantsSize;
        VkShaderStageFlags pushConstantStageFlags;
    };

    struct VulkanRasterCmd : public VulkanShaderCmd
    {
        VkRenderingInfo* renderingInfo;
        VkViewport* viewports;
        VkRect2D* scissors;
        uint32_t viewportCount;
        uint32_t scissorCount;
    };

    struct VulkanVertexCmd : public VulkanRasterCmd
    {
        VkBuffer* vertexBuffers;
        VkDeviceSize* vertexBufferOffsets;
        uint32_t vertexBufferCount = 0u;
    };

    struct VulkanIndexCmd : public VulkanVertexCmd
    {
        VkBuffer indexBuffer;
        VkDeviceSize indexBufferOffset;
        VkIndexType indexType;
    };

    struct VulkanDrawCmd : public VulkanVertexCmd
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };

    struct VulkanDrawIndirectCmd : public VulkanVertexCmd
    {
        VkBuffer indirectArguments;
        size_t indirectArgumentsOffset;
        uint32_t drawCount;
        uint32_t stride;
    };

    struct VulkanDrawIndexedCmd : public VulkanIndexCmd
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t firstInstance;
    };

    struct VulkanDrawIndexedIndirectCmd : public VulkanIndexCmd
    {
        VkBuffer indirectArguments;
        size_t indirectArgumentsOffset;
        uint32_t drawCount; 
        uint32_t stride;
    };

    struct VulkanDrawMeshTasksCmd : public VulkanRasterCmd
    {
        uint3 dimensions;
    };

    struct VulkanDrawMeshTasksIndirectCmd : public VulkanRasterCmd
    {
        VkBuffer indirectArguments;
        size_t indirectArgumentsOffset;
        uint32_t drawCount;
        uint32_t stride;
    };

    struct VulkanDrawMeshTasksIndirectCountCmd : public VulkanRasterCmd
    {
        VkBuffer indirectArguments;
        size_t indirectArgumentsOffset;
        VkBuffer countBuffer;
        size_t countBufferOffset;
        uint32_t maxDrawCount;
        uint32_t stride;
    };

    struct VulkanDispatchCmd : public VulkanShaderCmd
    {
        uint3 groupCounts;
    };

    struct VulkanDispatchIndirectCmd : public VulkanShaderCmd
    {
        VkBuffer indirectArguments;
        size_t indirectArgumentsOffset;
    };

    struct VulkanDispatchRaysCmd : public VulkanShaderCmd
    {
        VkStridedDeviceAddressRegionKHR addressRayGen;
        VkStridedDeviceAddressRegionKHR addressMiss;
        VkStridedDeviceAddressRegionKHR addressHit;
        VkStridedDeviceAddressRegionKHR addressCallable;
        uint3 dimensions;
    };

    struct VulkanBlitCmd : public VulkanCmd
    {
        VkImage src;
        VkImage dst;
        VulkanSwapchain* dstSwapchain;
        VkFormat srcFormat;
        VkFormat dstFormat;
        VkSampleCountFlags srcSamples;
        VkSampleCountFlags dstSamples;
        VkImageBlit region; 
        VkFilter filter;
    };

    struct VulkanBlitSwapchainToBufferCmd : public VulkanCmd
    {
        VulkanSwapchain* src;
        VkBuffer dst;
        VkBufferImageCopy region;
    };

    struct VulkanFillBufferCmd : public VulkanCmd
    {
        VkBuffer buffer;
        VkDeviceSize offset;
        VkDeviceSize size;
        uint32_t value;
    };

    struct VulkanClearImageCmd : public VulkanCmd
    {
        VkImage image;
        VkImageSubresourceRange range;
        VkClearValue clearValue;
    };

    struct VulkanUpdateBufferCmd : public VulkanCmd
    {
        VkBuffer buffer;
        VkDeviceSize offset;
        VkDeviceSize size;
        void* pData;
    };

    struct VulkanCopyBufferCmd : public VulkanCmd
    {
        VkBuffer src;
        VkBuffer dst;
        VkBufferCopy region;
    };

    struct VulkanCopyBufferToImageCmd : public VulkanCmd
    {
        VkBuffer src;
        VkImage dst;
        VkBufferImageCopy* regions;
        uint32_t regionCount;
    };

    struct VulkanAcquireNextImageCmd : public VulkanCmd
    {
        VulkanSwapchain* swapchain;
    };

    struct VulkanPresentImageCmd : public VulkanCmd
    {
        VulkanSwapchain* swapchain;
    };

    struct VulkanBeginDebugScopeCmd : public VulkanCmd
    {
        color color;
        char* label;
    };

    struct VulkanTimerCmd: public VulkanCmd
    {
        VkQueryPool queryPool;
        uint32_t queryIndex;
    };

    struct VulkanWriteAccelerationStructureCmd : public VulkanCmd
    {
        VkQueryPool queryPool = VK_NULL_HANDLE;
        VkAccelerationStructureKHR* queryHandles = nullptr;
        uint32_t queryStart = 0u;
        uint32_t queryCount = 0u;

        VkCopyAccelerationStructureInfoKHR* copyInfos = nullptr;
        uint32_t copyCount = 0u;

        VkAccelerationStructureBuildGeometryInfoKHR* BLASBuildInfos = nullptr;
        VkAccelerationStructureBuildRangeInfoKHR** BLASRangeInfos = nullptr;
        uint32_t BLASBuildCount = 0u;

        VkAccelerationStructureBuildGeometryInfoKHR TLASBuildInfo{};
        VkAccelerationStructureBuildRangeInfoKHR TLASRangeInfo{};
        bool TLASBuild = false;
    };

    struct VulkanRenderTarget
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

    struct VulkanGraphQueueState
    {
        VulkanPipelineCache::PipelineKey pipelineKey{};
        VkStridedDeviceAddressRegionKHR sbtAddresses[(uint32_t)RayTracingShaderGroup::EnumCount]{};
        VertexStreamElement vertexStreamLayout[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
        const VulkanBindHandle* vertexBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
        const VulkanBindHandle* indexBuffer = nullptr;
        VkIndexType indexType = VK_INDEX_TYPE_UINT16;
        VulkanRenderTarget renderTarget{};
        VkViewport viewports[PK_RHI_MAX_VIEWPORTS]{};
        uint32_t viewportCount = 0u;
        VkRect2D scissors[PK_RHI_MAX_VIEWPORTS]{};
        uint32_t scissorCount = 0u;

        VulkanDescriptorBinding descriptors[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        VkShaderStageFlagBits descriptorStageFlags = (VkShaderStageFlagBits)0;
        uint32_t descriptorCount = 0u;
        uint64_t dirtyFlags = 0ull;

        // Resolved state
        uint32_t vertexBufferCount = 0u;
        uint32_t resolvedViewportCount = 0u;
        VkDeviceSize descriptorSetOffset = 0ull;
        VkRenderingInfo* renderingInfo = nullptr;
        VkViewport* resolvedViewports = nullptr;
        VkRect2D* resolvedScissors = nullptr;
        const VulkanPipeline* pipeline = nullptr;
    };

    struct VulkanGraph : public RHIGraph
    {
        constexpr const static uint32_t MAX_SCOPES = 4u;

        enum DirtyFlags : uint64_t
        {
            STATE_DIRTY_RENDERTARGET = 1ull << 0ull,
            STATE_DIRTY_PIPELINE = 1ull << 1ull,
            STATE_DIRTY_SHADER = 1ull << 2ull,
            STATE_DIRTY_VERTEXBUFFERS = 1ull << 3ull,
            STATE_DIRTY_INDEXBUFFER = 1ull << 4ull,
            STATE_DIRTY_DESCRIPTORS = 1ull << 5ull,
            STATE_DIRTY_CONSTANTS = 1ull << 6ull,
            STATE_DIRTY_VIEWPORTS = 1ull << 7ull,
            STATE_DIRTY_SCISSORS = 1ull << 8ull,
        };

        VulkanGraph(const VulkanDriver* driver);
        ~VulkanGraph();

        FenceRef GetFenceRef() const final;
        void Execute() final;

        void SetBuffers(NameID name, RHIBuffer** buffers, const BufferIndexRange* ranges, size_t count) final;
        void SetTextures(NameID name, RHITexture** textures, const TextureViewRange* ranges, size_t count) final;
        void SetImages(NameID name, RHITexture** images, const TextureViewRange* ranges, size_t count) final;
        void SetSamplers(NameID name, const SamplerDescriptor* samplers, size_t count) final;
        void SetAccelerationStructures(NameID name, RHIAccelerationStructure** structures, size_t count) final;
        void SetBufferSet(NameID name, RHIBufferBindSet* bufferSet) final;
        void SetTextureSet(NameID name, RHITextureBindSet* textureSet) final;
        void SetConstant(NameID name, const void* data, uint32_t size) final;
        void SetKeyword(NameID name, bool value) final;

        RHIBuffer* AcquireStagingBuffer(size_t size) final;
        void ReleaseStagingBuffer(RHIBuffer* buffer) final;

        void SetViewPorts(QueueType queue, const uint4* rects, uint32_t count) final;
        void SetScissors(QueueType queue, const uint4* rects, uint32_t count) final;
        void SetRenderTarget(QueueType queue, const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers) final;
        void SetStageExcludeMask(QueueType queue, const ShaderStageFlags mask) final;
        void SetBlending(QueueType queue, const BlendParameters& blend) final;
        void SetRasterization(QueueType queue, const RasterizationParameters& rasterization) final;
        void SetDepthStencil(QueueType queue, const DepthStencilParameters& depthStencil) final;
        void SetMultisampling(QueueType queue, const MultisamplingParameters& multisampling) final;
        void SetShader(QueueType queue, const RHIShader* shader) final;
        void SetVertexBuffers(QueueType queue, const RHIBuffer** buffers, uint32_t count) final;
        void SetVertexStreams(QueueType queue, const VertexStreamElement* elements, uint32_t count) final;
        void SetIndexBuffer(QueueType queue, const RHIBuffer* buffer, size_t indexSize) final;
        void SetShaderBindingTable(QueueType queue, RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) final;

        void Draw(QueueType queue, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) final;
        void DrawIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawIndexed(QueueType queue, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final;
        void DrawIndexedIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawMeshTasks(QueueType queue, const uint3& dimensions) final;
        void DrawMeshTasksIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawMeshTasksIndirectCount(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) final;
        void Dispatch(QueueType queue, const uint3& dimensions) final;
        void DispatchIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset) final;
        void DispatchRays(QueueType queue, const uint3& dimensions) final;
        void Blit(QueueType queue, RHITexture* src, RHISwapchain* dst, FilterMode filter) final;
        void Blit(QueueType queue, RHISwapchain* src, RHIBuffer* dst) final;
        void Blit(QueueType queue, RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) final;
        void Clear(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, uint32_t value) final;
        void Clear(QueueType queue, RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value) final;
        void UpdateBuffer(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, const void* data) final;
        void CopyBuffer(QueueType queue, RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size) final;
        void CopyToTexture(QueueType queue, RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount) final;
        void InvalidateTexture(QueueType queue, RHITexture* texture) final;

        RHIAccelerationStructureBuilder* BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, size_t instanceLimit) final;
        void EndAccelerationStructureWrite(QueueType queue, RHIAccelerationStructureBuilder* builder) final;

        void AcquireNextImage(RHISwapchain* swapchain) final;
        void Present(RHISwapchain* swapchain) final;

        void BeginDebugScope(QueueType queue, const char* name, const color& color) final;
        void EndDebugScope(QueueType queue) final;
        void BeginTimer(QueueType queue, NameID name) final;
        void EndTimer(QueueType queue) final;

        void AddDispatchHint(QueueType queue) final;

    private:
        inline VulkanGraphQueueState* GetQueueState(QueueType queue) { return &m_queueStates[m_queueIndices[(uint32_t)queue]]; }
        inline uint32_t GetQueueFamily(QueueType queue) { return m_queueFamilies[(uint32_t)queue]; }
        inline uint32_t GetQueueIndex(QueueType queue) { return m_queueIndices[(uint32_t)queue]; }
        uint32_t GetQueueIndexFromFamily(uint32_t familyIndex) const;
        uint64_t AddBarrierPass(QueueType queue, bool isRaster);

        void RecordBufferAccess(const VulkanBindHandle* handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access);
        bool RecordImageAccess(QueueType queue, const VulkanBindHandle* handle, bool hasLayout, VkPipelineStageFlags2 stage, VkAccessFlags2 access);
        void RecordShaderCmd(QueueType queue, VulkanShaderCmd* cmd);
        void RecordRasterCmd(QueueType queue, VulkanRasterCmd* cmd);
        void RecordVertexCmd(QueueType queue, VulkanVertexCmd* cmd);
        void RecordIndexCmd(QueueType queue, VulkanIndexCmd* cmd);
        void RecordFlags(QueueType queue, VulkanCmd* cmd, VkPipelineStageFlags2 stageFlags);

        const VulkanDriver* m_driver;
        VulkanResourceState* m_resourceState;
        VulkanStagingArena m_stagingArena;
        VulkanDescriptorArena m_descriptorArena;
        VulkanTimerArena m_timerArena;
        PropertyBlock m_globalResources;
        VulkanGraphScope m_scopes[MAX_SCOPES]{};
        VulkanGraphQueueState m_queueStates[(uint32_t)QueueType::EnumCount]{};
        uint32_t m_queueFamilies[(uint32_t)QueueType::EnumCount]{};
        uint32_t m_queueIndices[(uint32_t)QueueType::EnumCount]{};
        uint32_t m_queueCount = 0ull;
        uint64_t m_invocationCounter = 0ull;
    };
}
