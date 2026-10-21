#pragma once
#include "Core/Base/Containers/PropertyBlock.h"
#include "Core/RHI/Vulkan/VulkanAccessRecorder.h"
#include "Core/RHI/Vulkan/VulkanShader.h"
#include "Core/RHI/Vulkan/VulkanQueue.h"
#include "Core/RHI/RHIGraph.h"

namespace PK
{
    struct VulkanDriver;
    struct VulkanAccessRecorder;
    struct VulkanSwapchain;
    struct VulkanStagingArena;
    struct VulkanDescriptorHeap;
    struct VulkanTimerArena;
    struct VulkanCommandBuffer;


    struct VulkanCmd : public RHICmd
    {
        uint64_t passIndex;
        const VkImageMemoryBarrier2* imageTransitions = nullptr;
        const VkMemoryBarrier2* memoryBarrier = nullptr;
        VkPipelineStageFlags2 waitStageMask = 0ull;
        uint32_t imageTransitionCount = 0u;
        uint32_t dependencyMask = 0u;
    };

    struct VulkanShaderCmd : public VulkanCmd
    {
        const VulkanShader* shader;
        void* pushConstants;
        uint2 descriptorHeapOffsets;
        uint32_t pushConstantSize;
        uint16_t excludeStageMask;
    };

    struct VulkanRasterCmd : public VulkanShaderCmd
    {
        VkRenderingInfo* renderingInfo;
        const VulkanDynamicState* dynamicState;
    };

    struct VulkanVertexCmd : public VulkanRasterCmd
    {
        VkBindVertexBuffer3InfoKHR* vertexBuffers;
        uint32_t vertexBufferCount;
    };

    struct VulkanIndexCmd : public VulkanVertexCmd
    {
        VkBindIndexBuffer3InfoKHR indexInfo;
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
        VkDrawIndirect2InfoKHR indirectInfo;
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
        VkDrawIndirect2InfoKHR indirectInfo;
    };

    struct VulkanDrawMeshTasksCmd : public VulkanRasterCmd
    {
        uint3 dimensions;
    };

    struct VulkanDrawMeshTasksIndirectCmd : public VulkanRasterCmd
    {
        VkDrawIndirect2InfoKHR indirectInfo;
    };

    struct VulkanDrawMeshTasksIndirectCountCmd : public VulkanRasterCmd
    {
        VkDrawIndirectCount2InfoKHR indirectCountInfo;
    };

    struct VulkanDispatchCmd : public VulkanShaderCmd
    {
        uint3 groupCounts;
    };

    struct VulkanDispatchIndirectCmd : public VulkanShaderCmd
    {
        VkDispatchIndirect2InfoKHR indirectInfo;
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
        VkDeviceMemoryImageCopyKHR region;
    };

    struct VulkanFillBufferCmd : public VulkanCmd
    {
        VkDeviceAddressRangeKHR range;
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
        VkDeviceAddressRangeKHR range;
        void* pData;
    };

    struct VulkanCopyBufferCmd : public VulkanCmd
    {
        VkDeviceMemoryCopyKHR region;
    };

    struct VulkanCopyBufferToImageCmd : public VulkanCmd
    {
        VkImage dst;
        VkDeviceMemoryImageCopyKHR* regions;
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

    struct VulkanDebugCmd : public VulkanCmd
    {
        color color;
        char* label;
    };

    struct VulkanTimerCmd : public VulkanCmd
    {
        NameID name;
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
            VulkanResourceView target{};
            VulkanResourceView resolve{};
            VkFormat format = VK_FORMAT_UNDEFINED;
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

    struct VulkanGraphScope
    {
        FenceRef fence;
        uint64_t stagingRangeIndex = 0ull;
        uint64_t descriptorRangeIndex = 0ull;
        uint64_t timerTimelineIndex = 0ull;
        uint64_t invocationIndex = 0ull;
        uint64_t lastPassCounter = 0ull;
    };


    struct VulkanGraph : public RHIGraph
    {
        enum DirtyFlags : uint32_t
        {
            STATE_DIRTY_RENDERTARGET = 1ull << 0ull,
            STATE_DIRTY_SHADER = 1ull << 1ull,
            STATE_DIRTY_VERTEXBUFFERS = 1ull << 2ull,
            STATE_DIRTY_DESCRIPTORS = 1ull << 3ull,
            STATE_DIRTY_DYNAMIC_STATE = 1u << 4u
        };

        VulkanGraph(const VulkanDriver* driver);
        ~VulkanGraph();

        // Vulkan specific image registrations into the resource tracking system.
        inline void RegisterImage(VkImage image, VkImageAspectFlags aspect) { m_accessRecorder.RegisterImage(image, aspect); }
        inline void UnregisterImage(VkImage image) { m_accessRecorder.UnregisterImage(image); }

        void SetBuffers(NameID name, RHIBuffer** buffers, const BufferIndexRange* ranges, size_t count) final;
        void SetTextures(NameID name, RHITexture** textures, const TextureViewRange* ranges, size_t count) final;
        void SetImages(NameID name, RHITexture** images, const TextureViewRange* ranges, size_t count) final;
        void SetSamplers(NameID name, const SamplerDescriptor* samplers, size_t count) final;
        void SetAccelerationStructures(NameID name, RHIAccelerationStructure** structures, size_t count) final;
        void SetConstant(NameID name, const void* data, uint32_t size) final;
        void SetKeyword(NameID name, bool value) final;

        uint32_t MakeResident(RHITexture* texture) final;
        uint32_t MakeResident(RHIBuffer* buffer) final;
        uint32_t MakeResident(const SamplerDescriptor& sampler) final;
        uint3 GetResidentTextureSize(uint32_t bindlessIndex) final;

        RHIBuffer* AcquireStagingBuffer(size_t size) final;
        void ReleaseStagingBuffer(RHIBuffer* buffer) final;

        void SetViewPorts(const uint4* rects, uint32_t count) final;
        void SetScissors(const uint4* rects, uint32_t count) final;
        void SetRenderTarget(const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers) final;
        void SetBlending(const BlendParameters& blend) final;
        void SetRasterization(const RasterizationParameters& rasterization) final;
        void SetDepthStencil(const DepthStencilParameters& depthStencil) final;
        void SetMultisampling(const MultisamplingParameters& multisampling) final;
        void SetStageExcludeMask(const ShaderStageFlags mask) final;
        void SetShader(const RHIShader* shader) final;
        void SetVertexBuffers(const RHIBuffer** buffers, uint32_t count) final;
        void SetVertexStreams(const VertexStreamElement* elements, uint32_t count) final;
        void SetIndexBuffer(const RHIBuffer* buffer, size_t indexSize) final;
        void SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) final;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) final;
        void DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final;
        void DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawMeshTasks(const uint3& dimensions) final;
        void DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) final;
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

        RHIAccelerationStructureBuilder* BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, size_t instanceLimit) final;
        void EndAccelerationStructureWrite(QueueType queue, RHIAccelerationStructureBuilder* builder) final;

        void AcquireNextImage(RHISwapchain* swapchain) final;
        void Present(RHISwapchain* swapchain) final;

        void BeginDebugScope(const char* name, const color& color) final;
        void EndDebugScope() final;
        void BeginTimer(NameID name) final;
        void EndTimer() final;
        
        PropertyBlock* GetResourceState() final { return &m_globalResources; }
        ConstBufferView<RHITimerScope> GetTimers() const final;
        FenceRef GetFenceRef() const final;
        void Execute() final;

    private:
        void BeginGraph();
        VulkanGraphScope& EndGraph();

        void ApplyDebugScopes(uint32_t queueIndex, struct VulkanGraphDebugScopes* scopes, uint32_t debugTarget, uint32_t timerTarget);
        void SubmitQueue(uint32_t queueIndex, VkSemaphore signal, struct VulkanGraphDebugScopes* scopes);

        void AddResidencyBarrierPass();
        uint64_t AddBarrierPass(QueueType queue, bool isRaster);
        void RecordMemoryAccess(QueueType queue, const VulkanResourceView& view, VkPipelineStageFlags2 stage, VkAccessFlags2 access);
        void RecordImageAccess(QueueType queue, const VulkanResourceView& view, VkPipelineStageFlags2 stage, VkAccessFlags2 access);
        
        void RecordShaderCmd(QueueType queue, VulkanShaderCmd* cmd);
        void RecordRasterCmd(VulkanRasterCmd* cmd);
        void RecordVertexCmd(VulkanVertexCmd* cmd);
        void RecordIndexCmd(VulkanIndexCmd* cmd);

        PropertyBlock m_globalResources;
        VulkanAccessRecorder m_accessRecorder;
        const VulkanDriver* m_driver;
        VulkanStagingArena* m_stagingArena;
        VulkanDescriptorHeap* m_descriptorHeap;
        VulkanTimerArena* m_timerArena;
        mutable VulkanGraphScope m_scopes[PK_VK_MAX_GRAPH_SCOPES]{};
        uint64_t m_invocationCounter;
        
        struct PipelineState
        {
            VersionHandle<VulkanShader> shader{};
            VulkanDynamicState dynamicState{};
            VkStridedDeviceAddressRegionKHR sbtAddresses[(uint32_t)RayTracingShaderGroup::EnumCount]{};
            VertexStreamElement vertexStreamLayout[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            VkVertexInputBindingDescription2EXT vertexStreams[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            VkVertexInputAttributeDescription2EXT vertexAttributes[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            VulkanResourceView vertexBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
            VulkanResourceView indexBuffer{};
            VkIndexType indexType = VK_INDEX_TYPE_UINT16;
            VulkanRenderTarget renderTarget{};

            VulkanDescriptorBinding descriptors[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
            uint32_t descriptorCount = 0u;
            uint32_t dirtyFlags = 0u;
            uint16_t excludeStageMask = 0u;
            uint32_t coalescingQueue = ~0u;
            uint64_t residencyPass = 0ull;
            uint2 descriptorHeapOffsets = PK_UINT2_ZERO;

            // Resolved state
            uint32_t vertexBufferCount = 0u;
            VkRenderingInfo* renderingInfo = nullptr;
            VulkanDynamicState* resolvedDynamicState = nullptr;
            VkBindVertexBuffer3InfoKHR* resolvedVertexBuffers = nullptr;
            VulkanResourceView resolvedVertexViews[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
        } 
        m_pipelineState;
    };
}
