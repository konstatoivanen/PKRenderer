#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/NativeInterface.h"
#include "Core/Utilities/BufferView.h"
#include "Core/Utilities/Ref.h"
#include "Core/RHI/RHI.h"
#include "Core/RHI/Structs.h"

namespace PK
{
    struct RHIWindow : public NoCopy, public NativeInterface<RHIWindow>
    {
        constexpr static uint32_t MIN_SIZE = 256u;
        virtual ~RHIWindow() = 0;
        virtual uint3 GetResolution() const = 0;
        virtual uint4 GetRect() const = 0;
        virtual float GetAspectRatio() const = 0;
        virtual bool IsAlive() const = 0;
        virtual bool IsMinimized() const = 0;
        virtual bool IsVSync() const = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void SetFrameFence(const FenceRef& fence) = 0;
        virtual void SetCursorVisible(bool value) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual void SetOnResizeCallback(const std::function<void(int width, int height)>& callback) = 0;
        virtual void SetOnCloseCallback(const std::function<void()>& callback) = 0;
        virtual void PollEvents() const = 0;
        virtual void WaitEvents() const = 0;
        virtual void* GetNativeWindow() const = 0;
    };

    struct RHITexture : public NoCopy, public NativeInterface<RHITexture>
    {
        virtual ~RHITexture() = 0;
        virtual void SetSampler(const SamplerDescriptor& sampler) = 0;
        virtual bool Validate(const uint3& resolution) = 0;
        virtual bool Validate(const uint32_t levels, const uint32_t layers) = 0;
        virtual bool Validate(const TextureDescriptor& descriptor) = 0;
        virtual const TextureDescriptor& GetDescriptor() const = 0;

        constexpr TextureUsage GetUsage() const { return GetDescriptor().usage; }
        constexpr bool IsConcurrent() const { return (GetUsage() & TextureUsage::Concurrent) != 0; }
        constexpr bool IsTracked() const { return (GetUsage() & TextureUsage::ReadOnly) == 0; }
        constexpr const SamplerDescriptor& GetSamplerDescriptor() const { return GetDescriptor().sampler; }
        constexpr uint4 GetRect() const { return { 0, 0, GetDescriptor().resolution.x, GetDescriptor().resolution.y }; }
        constexpr uint3 GetResolution() const { return GetDescriptor().resolution; }
        constexpr uint32_t GetLevels() const { return GetDescriptor().levels; }
        constexpr uint32_t GetLayers() const { return GetDescriptor().layers; }
    };

    struct RHIBuffer : public NoCopy, public NativeInterface<RHIBuffer>
    {
        virtual ~RHIBuffer() = 0;
        virtual const void* BeginRead(size_t offset, size_t size) = 0;
        virtual void EndRead() = 0;
        virtual size_t SparseAllocate(const size_t size, QueueType type) = 0;
        virtual void SparseAllocateRange(const BufferIndexRange& range, QueueType type) = 0;
        virtual void SparseDeallocate(const BufferIndexRange& range) = 0;
        virtual bool Validate(size_t size) = 0;
        virtual size_t GetSize() const = 0;
        virtual size_t GetCapacity() const = 0;
        virtual BufferUsage GetUsage() const = 0;

        template<typename T>
        ConstBufferView<T> BeginRead()
        {
            return { reinterpret_cast<const T*>(BeginRead(0, GetCapacity())), GetCapacity() / sizeof(T) };
        }

        template<typename T>
        ConstBufferView<T> BeginRead(size_t offset, size_t count)
        {
            return { reinterpret_cast<const T*>(BeginRead(offset * sizeof(T), count * sizeof(T))), count };
        }

        template<typename T> inline bool Validate(size_t count) { return Validate(sizeof(T) * count); }

        template<typename T>
        constexpr size_t GetCount() const { return GetSize() / sizeof(T); }
        constexpr bool IsSparse() const { return (GetUsage() & BufferUsage::Sparse) != 0; }
        constexpr bool IsConcurrent() const { return (GetUsage() & BufferUsage::Concurrent) != 0u; }
        constexpr BufferIndexRange GetFullRange() const { return { 0ull, GetSize() }; }
    };

    struct RHIAccelerationStructure : public NoCopy, public NativeInterface<RHIAccelerationStructure>
    {
        virtual ~RHIAccelerationStructure() = 0;
        virtual void BeginWrite(QueueType queue, uint32_t instanceLimit) = 0;
        virtual void AddInstance(AccelerationStructureGeometryInfo& geometry, const float3x4& matrix) = 0;
        virtual void EndWrite() = 0;
        virtual uint32_t GetInstanceCount() const = 0;
        virtual uint32_t GetSubStructureCount() const = 0;
    };

    template<typename T>
    struct RHIBindArray : public NoCopy, public NativeInterface<RHIBindArray<T>>
    {
        virtual ~RHIBindArray() = 0;
        virtual int32_t Add(T* value, void* bindInfo) = 0;
        virtual int32_t Add(T* value) = 0;
        virtual void Clear() = 0;
    };

    struct RHIShader : public NoCopy, public NativeInterface<RHIShader>
    {
        virtual ~RHIShader() = 0;
        virtual const ShaderVertexInputLayout& GetVertexLayout() const = 0;
        virtual const ShaderPushConstantLayout& GetPushConstantLayout() const = 0;
        virtual const ShaderResourceLayout& GetResourceLayout(uint32_t set) const = 0;
        virtual ShaderStageFlags GetStageFlags() const = 0;
        virtual const uint3& GetGroupSize() const = 0;
        virtual ShaderBindingTableInfo GetShaderBindingTableInfo() const = 0;
        inline bool IsGraphics() const { return (GetStageFlags() & ShaderStageFlags::StagesGraphics) != 0; }
        inline bool HasRayTracingShaderGroup(RayTracingShaderGroup group) const { return (PK_RHI_RAYTRACING_GROUP_SHADER_STAGE[(uint32_t)group] & GetStageFlags()) != 0; }
    };

    struct RHICommandBuffer : public NoCopy, public NativeInterface<RHICommandBuffer>
    {
        virtual FenceRef GetFenceRef() const = 0;
        virtual void SetRenderTarget(const uint3& resolution) = 0;
        virtual void SetRenderTarget(RHITexture* const* renderTarget, RHITexture* const* resolveTargets, const TextureViewRange* ranges, uint32_t count) = 0;
        virtual void ClearColor(const color& color, uint32_t index) = 0;
        virtual void ClearDepth(float depth, uint32_t stencil) = 0;
        virtual void DiscardColor(uint32_t index) = 0;
        virtual void DiscardDepth() = 0;

        virtual void SetViewPorts(const uint4* rects, uint32_t count) = 0;
        virtual void SetScissors(const uint4* rects, uint32_t count) = 0;
        virtual void SetUnorderedOverlap(bool value) = 0;

        virtual void SetStageExcludeMask(const ShaderStageFlags mask) = 0;
        virtual void SetBlending(const BlendParameters& blend) = 0;
        virtual void SetRasterization(const RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(const DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(const MultisamplingParameters& multisampling) = 0;

        virtual void SetShader(const RHIShader* shader) = 0;
        virtual void SetVertexBuffers(const RHIBuffer** buffers, uint32_t count) = 0;
        virtual void SetVertexStreams(const VertexStreamElement* elements, uint32_t count) = 0;
        virtual void SetIndexBuffer(const RHIBuffer* buffer, ElementType indexFormat) = 0;
        virtual void SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasks(const uint3& dimensions) = 0;
        virtual void DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
        virtual void Dispatch(const uint3& dimensions) = 0;
        virtual void DispatchRays(const uint3& dimensions) = 0;

        virtual void Blit(RHITexture* src, RHIWindow* dst, FilterMode filter) = 0;
        virtual void Blit(RHIWindow* src, RHIBuffer* dst) = 0;
        virtual void Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) = 0;

        virtual void Clear(RHIBuffer* dst, size_t offset, size_t size, uint32_t value) = 0;
        virtual void Clear(RHITexture* dst, const TextureViewRange& range, const uint4& value) = 0;

        virtual void UpdateBuffer(RHIBuffer* dst, size_t offset, size_t size, void* data) = 0;
        virtual void* BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t size) = 0;
        virtual void EndBufferWrite(RHIBuffer* buffer) = 0;

        virtual void UploadTexture(RHITexture* texture, const void* data, size_t size, TextureUploadRange* ranges, uint32_t rangeCount) = 0;
        virtual void UploadTexture(RHITexture* texture, const void* data, size_t size, uint32_t level, uint32_t layer) = 0;

        virtual void BeginDebugScope(const char* name, const color& color) = 0;
        virtual void EndDebugScope() = 0;
    };

    struct RHIQueueSet : public NoCopy, public NativeInterface<RHIQueueSet>
    {
        virtual ~RHIQueueSet() = 0;
        constexpr static const uint32_t MAX_DEPENDENCIES = (uint32_t)QueueType::MaxCount;
        virtual RHICommandBuffer* GetCommandBuffer(QueueType type) = 0;
        virtual FenceRef GetFenceRef(QueueType type, int32_t submitOffset = 0) = 0;
        virtual RHICommandBuffer* Submit(QueueType type) = 0;
        virtual void Sync(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Wait(QueueType from, QueueType to, int32_t submitOffset = 0) = 0;
        virtual void Transfer(QueueType from, QueueType to) = 0;
        inline void Submit(QueueType type, RHICommandBuffer** cmd) { *cmd = Submit(type); }
    };

    struct RHIDriver : public NoCopy, public NativeInterface<RHIDriver>
    {
        RHIDriver() { if (s_instance != nullptr) throw std::exception("Trying initialize multiple RHI drivers!"); s_instance = this; }

        virtual ~RHIDriver() = 0;
        virtual RHIAPI GetAPI() const = 0;
        virtual RHIQueueSet* GetQueues() const = 0;
        virtual RHIDriverMemoryInfo GetMemoryInfo() const = 0;
        virtual std::string GetDriverHeader() const = 0;
        virtual size_t GetBufferOffsetAlignment(BufferUsage usage) const = 0;
        virtual BuiltInResources* GetBuiltInResources() = 0;
        virtual PropertyBlock* GetResourceState() = 0;

        virtual RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) = 0;
        virtual RHITextureBindArrayRef CreateTextureBindArray(size_t capacity) = 0;
        virtual RHIBufferBindArrayRef CreateBufferBindArray(size_t capacity) = 0;
        virtual RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) = 0;
        virtual RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) = 0;
        virtual RHIShaderScope CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name) = 0;
        virtual RHIWindowScope CreateWindowScope(const WindowDescriptor& descriptor) = 0;

        virtual void SetBuffer(NameID name, RHIBuffer* buffer, const BufferIndexRange& range) = 0;
        virtual void SetTexture(NameID name, RHITexture* texture, const TextureViewRange& range) = 0;
        virtual void SetBufferArray(NameID name, RHIBufferBindArray* bufferArray) = 0;
        virtual void SetTextureArray(NameID name, RHITextureBindArray* textureArray) = 0;
        virtual void SetImage(NameID name, RHITexture* texture, const TextureViewRange& range) = 0;
        virtual void SetSampler(NameID name, const SamplerDescriptor& sampler) = 0;
        virtual void SetAccelerationStructure(NameID name, RHIAccelerationStructure* structure) = 0;
        virtual void SetConstant(NameID name, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(NameID name, bool value) = 0;

        virtual void WaitForIdle() const = 0;
        virtual void GC() = 0;

        static inline RHIDriver* Get() { return s_instance; }

        protected: inline static RHIDriver* s_instance = nullptr;
    };
}