#pragma once
#include "Core/Base/NoCopy.h"
#include "Core/Base/Containers/FixedArena.h"
#include "Core/Base/TypeMeta.h"
#include "RHI.h"

namespace PK
{
    template <typename T>
    struct RHICmdTraits;

    template <typename T, typename TCtx, typename TCmd>
    struct RHICmdTraits<void(T::*)(TCtx, TCmd*) const>
    {
        using TContext = TCtx;
        using TCommand = TCmd;
    };

    struct RHICmd
    {
        void (*call)(void*, RHICmd*);
        RHICmd* next;
        QueueType queue;
    };

    struct RHIGraph : public NoCopy
    {
        template<typename T>
        T* Allocate(size_t count)
        {
            return m_arena.Allocate<T>(count);
        }

        template<typename T, typename ... Args>
        T* AllocateNew(Args&& ... args)
        {
            return m_arena.New<T>(PK::Forward<Args>(args)...);
        }

        template <typename TLambda>
        auto* AllocateCommand(QueueType queue, TLambda&&)
        {
            using TFunc = decltype(&TRemoveCVRef_T<TLambda>::operator());
            using TCmd = typename RHICmdTraits<TFunc>::TCommand;
            using TCtx = typename RHICmdTraits<TFunc>::TContext;
            static_assert(TIsBaseOf<RHICmd, TCmd>, "RHI Graph commands must inherit from RHICmd");

            auto cmd = m_arena.Allocate<TCmd>(1u);
            cmd->queue = queue;
            cmd->call = [](void* ctx, RHICmd* cmd) { TLambda{}(static_cast<TCtx>(ctx), static_cast<TCmd*>(cmd)); };
            m_commandCount++;

            if (m_head)
            {
                m_head->next = cmd;
            }
            
            m_head = cmd;
            return cmd;
        }

        inline RHICmd* GetCommands() 
        { 
            return reinterpret_cast<RHICmd*>(m_arena.GetData()); 
        }

        inline size_t GetCommandCount() const 
        {
            return m_commandCount; 
        }

        inline void Clear() 
        {
            m_arena.Clear();
            m_commandCount = 0u;
        }

        virtual ~RHIGraph() = 0;
        virtual FenceRef GetFenceRef() const = 0;
        virtual void Execute() = 0;

        virtual void SetBuffers(NameID name, RHIBuffer** buffers, const BufferIndexRange* ranges, size_t count) = 0;
        virtual void SetTextures(NameID name, RHITexture** textures, const TextureViewRange* ranges, size_t count) = 0;
        virtual void SetImages(NameID name, RHITexture** images, const TextureViewRange* ranges, size_t count) = 0;
        virtual void SetSamplers(NameID name, const SamplerDescriptor* samplers, size_t count) = 0;
        virtual void SetAccelerationStructures(NameID name, RHIAccelerationStructure** structures, size_t count) = 0;
        virtual void SetBufferSet(NameID name, RHIBufferBindSet* bufferSet) = 0;
        virtual void SetTextureSet(NameID name, RHITextureBindSet* textureSet) = 0;
        virtual void SetConstant(NameID name, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(NameID name, bool value) = 0;

        virtual RHIBuffer* AcquireStagingBuffer(size_t size) = 0;
        virtual void ReleaseStagingBuffer(RHIBuffer* buffer) = 0;

        virtual void SetViewPorts(QueueType queue, const uint4* rects, uint32_t count) = 0;
        virtual void SetScissors(QueueType queue, const uint4* rects, uint32_t count) = 0;
        virtual void SetRenderTarget(QueueType queue, const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers) = 0;
        virtual void SetStageExcludeMask(QueueType queue, const ShaderStageFlags mask) = 0;
        virtual void SetBlending(QueueType queue, const BlendParameters& blend) = 0;
        virtual void SetRasterization(QueueType queue, const RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(QueueType queue, const DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(QueueType queue, const MultisamplingParameters& multisampling) = 0;
        virtual void SetShader(QueueType queue, const RHIShader* shader) = 0;
        virtual void SetVertexBuffers(QueueType queue, const RHIBuffer** buffers, uint32_t count) = 0;
        virtual void SetVertexStreams(QueueType queue, const VertexStreamElement* elements, uint32_t count) = 0;
        virtual void SetIndexBuffer(QueueType queue, const RHIBuffer* buffer, size_t indexSize) = 0;
        virtual void SetShaderBindingTable(QueueType queue, RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) = 0;

        virtual void Draw(QueueType queue, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawIndexed(QueueType queue, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasks(QueueType queue, const uint3& dimensions) = 0;
        virtual void DrawMeshTasksIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasksIndirectCount(QueueType queue, const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
        virtual void Dispatch(QueueType queue, const uint3& dimensions) = 0;
        virtual void DispatchIndirect(QueueType queue, const RHIBuffer* indirectArguments, size_t offset) = 0;
        virtual void DispatchRays(QueueType queue, const uint3& dimensions) = 0;
        virtual void Blit(QueueType queue, RHITexture* src, RHISwapchain* dst, FilterMode filter) = 0;
        virtual void Blit(QueueType queue, RHISwapchain* src, RHIBuffer* dst) = 0;
        virtual void Blit(QueueType queue, RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) = 0;
        virtual void Clear(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, uint32_t value) = 0;
        virtual void Clear(QueueType queue, RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value) = 0;
        virtual void UpdateBuffer(QueueType queue, RHIBuffer* dst, size_t offset, size_t size, const void* data) = 0;
        virtual void CopyBuffer(QueueType queue, RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size) = 0;
        virtual void CopyToTexture(QueueType queue, RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount) = 0;
        virtual void InvalidateTexture(QueueType queue, RHITexture* texture) = 0;
        
        virtual RHIAccelerationStructureBuilder* BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, size_t instanceLimit) = 0;
        virtual void EndAccelerationStructureWrite(QueueType queue, RHIAccelerationStructureBuilder* builder) = 0;
        
        virtual void AcquireNextImage(RHISwapchain* swapchain) = 0;
        virtual void Present(RHISwapchain* swapchain) = 0;
        
        virtual void BeginDebugScope(QueueType queue, const char* name, const color& color) = 0;
        virtual void EndDebugScope(QueueType queue) = 0;
        virtual void BeginTimer(QueueType queue, NameID name) = 0;
        virtual void EndTimer(QueueType queue) = 0;
        
        virtual void AddDispatchHint(QueueType queue) = 0;
    
    private:
        FixedArena<33554432ull> m_arena;
        RHICmd* m_head;
        size_t m_commandCount;
    };
}
