#pragma once
#include "Core/Base/Containers/BufferView.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Containers/FixedArena.h"
#include "Core/Base/TypeMeta.h"
#include "Core/Base/NoCopy.h"
#include "Core/RHI/Structs.h"
#include "Core/RHI/RHI.h"

namespace PK
{
    struct RHISwapchain : public NoCopy
    {
        virtual ~RHISwapchain() = 0;
        virtual void SetDesiredResolution(const uint2& resolution) = 0;
        virtual void SetDesiredFormat(TextureFormat format) = 0;
        virtual void SetDesiredColorSpace(ColorSpace colorSpace) = 0;
        virtual void SetDesiredVSyncMode(VSyncMode vsyncMode) = 0;
        virtual bool AcquireFullScreen(const void* nativeMonitor) = 0;
        virtual bool IsFullScreen() const = 0;
        virtual uint3 GetResolution() const = 0;
        virtual TextureFormat GetFormat() const = 0;
        virtual ColorSpace GetColorSpace() const = 0;
        virtual VSyncMode GetVSyncMode() const = 0;

        inline uint32_t GetWidth() const { return GetResolution().x; }
        inline uint32_t GetHeight() const { return GetResolution().y; }
        inline float GetAspectRatio() const { return (float)GetWidth() / (float)GetHeight(); }
    };

    struct RHITexture : public NoCopy
    {
        virtual ~RHITexture() = 0;
        virtual void SetSampler(const SamplerDescriptor& sampler) = 0;
        virtual const TextureDescriptor& GetDescriptor() const = 0;
        virtual const char* GetDebugName() const = 0;
        virtual const void* GetNativeView(const TextureViewRange& range, TextureViewMode viewMode) = 0;

        inline TextureFormat GetFormat() const { return GetDescriptor().format; }
        inline TextureUsage GetUsage() const { return GetDescriptor().usage; }
        inline const SamplerDescriptor& GetSamplerDescriptor() const { return GetDescriptor().sampler; }
        inline uint4 GetRect() const { return { 0, 0, GetDescriptor().resolution.x, GetDescriptor().resolution.y }; }
        inline uint3 GetResolution() const { return GetDescriptor().resolution; }
        inline float3 GetTexelSize() const { return 1.0f / float3(GetDescriptor().resolution); }
        inline uint32_t GetLevels() const { return GetDescriptor().levels; }
        inline uint32_t GetLayers() const { return GetDescriptor().layers; }

        template<typename T> 
        inline const T* GetNativeView(const TextureViewRange& range, TextureViewMode viewMode) { return static_cast<const T*>(GetNativeView(range, viewMode)); }
    };

    struct RHIBuffer : public NoCopy
    {
        virtual ~RHIBuffer() = 0;
        virtual size_t GetSize() const = 0;
        virtual BufferUsage GetUsage() const = 0;
        virtual const char* GetDebugName() const = 0;
        virtual uint64_t GetDeviceAddress() const = 0;
        virtual const void* GetNativeView() const = 0;
        virtual void* GetMappedPtr() const = 0;
        template<typename T> inline const T* GetNativeView() const { return static_cast<const T*>(GetNativeView()); }
        template<typename T> inline T* GetMappedPtr() const { return reinterpret_cast<T*>(GetMappedPtr()); }
    };

    struct RHIAccelerationStructure : public NoCopy
    {
        virtual ~RHIAccelerationStructure() = 0;
        virtual uint32_t GetInstanceCount() const = 0;
        virtual uint32_t GetSubStructureCount() const = 0;
        virtual FenceRef GetLastBuildFenceRef() const = 0;
    };

    struct RHIAccelerationStructureBuilder : public NoCopy
    {
        virtual void AddInstance(const RayTracingGeometryInfo& geometry, const float3x4& matrix) = 0;
    };

    template<typename T>
    struct RHIBindSet : public NoCopy
    {
        // @TODO MSVC doesnt allow this to be specialized but clang does?
        #if defined(__clang__)
        virtual ~RHIBindSet() = 0;
        #else
        virtual ~RHIBindSet() {};
        #endif
        virtual int32_t Add(T* value, void* bindInfo) = 0;
        virtual int32_t Add(T* value) = 0;

        virtual uint3 GetBoundTextureSize(uint32_t index) const = 0;
        
        virtual void Clear() = 0;
    };

    struct RHIShader : public NoCopy
    {
        virtual ~RHIShader() = 0;
        virtual const ShaderVertexInputLayout& GetVertexLayout() const = 0;
        virtual const ShaderPushConstantLayout& GetPushConstantLayout() const = 0;
        virtual const ShaderResourceLayout& GetResourceLayout() const = 0;
        virtual ShaderStageFlags GetStageFlags() const = 0;
        virtual const uint3& GetGroupSize() const = 0;
        virtual ShaderBindingTableInfo GetShaderBindingTableInfo() const = 0;

        inline bool IsGraphics() const { return (GetStageFlags() & ShaderStageFlags::StagesGraphics) != 0; }

        inline bool HasRayTracingShaderGroup(RayTracingShaderGroup group) const 
        {
            return (PK_RHI_RAYTRACING_GROUP_SHADER_STAGE[(uint32_t)group] & GetStageFlags()) != 0; 
        }
    };

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

        virtual ~RHIGraph() = 0;
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

        virtual void SetViewPorts(const uint4* rects, uint32_t count) = 0;
        virtual void SetScissors(const uint4* rects, uint32_t count) = 0;
        virtual void SetRenderTarget(const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers) = 0;
        virtual void SetBlending(const BlendParameters& blend) = 0;
        virtual void SetRasterization(const RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(const DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(const MultisamplingParameters& multisampling) = 0;
        virtual void SetStageExcludeMask(const ShaderStageFlags mask) = 0;
        virtual void SetShader(const RHIShader* shader) = 0;
        virtual void SetVertexBuffers(const RHIBuffer** buffers, uint32_t count) = 0;
        virtual void SetVertexStreams(const VertexStreamElement* elements, uint32_t count) = 0;
        virtual void SetIndexBuffer(const RHIBuffer* buffer, size_t indexSize) = 0;
        virtual void SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasks(const uint3& dimensions) = 0;
        virtual void DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
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

        virtual RHIAccelerationStructureBuilder* BeginAccelerationStructureWrite(RHIAccelerationStructure* structure, size_t instanceLimit) = 0;
        virtual void EndAccelerationStructureWrite(QueueType queue, RHIAccelerationStructureBuilder* builder) = 0;

        virtual void AcquireNextImage(RHISwapchain* swapchain) = 0;
        virtual void Present(RHISwapchain* swapchain) = 0;

        virtual void BeginDebugScope(const char* name, const color& color) = 0;
        virtual void EndDebugScope() = 0;
        virtual void BeginTimer(NameID name) = 0;
        virtual void EndTimer() = 0;
        virtual void AddDispatchHint(QueueType queue) = 0;

        virtual FenceRef GetFenceRef() const = 0;
        virtual ConstBufferView<RHITimerScope> GetTimers() const = 0;
        virtual PropertyBlock* GetResourceState() = 0;
        virtual void Execute() = 0;
    
    protected:
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
            m_head = nullptr;
        }

    private:
        FixedArena<8388608ull> m_arena;
        RHICmd* m_head;
        size_t m_commandCount;
    };

    struct RHIDriver : public NoCopy
    {
        RHIDriver() { Memory::Assert(s_instance == nullptr, "Attempting to create multiple RHIs"); s_instance = this; }

        virtual ~RHIDriver() = 0;
        virtual RHIAPI GetAPI() const = 0;
        virtual RHIGraph* GetGraph() const = 0;
        virtual RHIDriverMemoryInfo GetMemoryInfo() const = 0;
        virtual FixedString32 GetDriverHeader() const = 0;
        virtual size_t GetBufferAlignment() const = 0;
        virtual BuiltInResources* GetBuiltInResources() = 0;

        virtual RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) = 0;
        virtual RHITextureBindSetRef CreateTextureBindSet(size_t capacity) = 0;
        virtual RHIBufferBindSetRef CreateBufferBindSet(size_t capacity) = 0;
        virtual RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) = 0;
        virtual RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) = 0;
        virtual RHIShaderRef CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) = 0;
        virtual RHISwapchainScope CreateSwapchain(const SwapchainDescriptor& descriptor) = 0;

        virtual void WaitForIdle() const = 0;
        virtual void GC() = 0;

        static inline RHIDriver* Get() { return s_instance; }

        protected: inline static RHIDriver* s_instance = nullptr;
    };
}
