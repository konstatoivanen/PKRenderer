#pragma once
#include <exception>
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics::RHI
{
    struct RHIDriver : public PK::Utilities::NoCopy, public PK::Utilities::NativeInterface<RHIDriver>
    {
        RHIDriver() { if (s_instance != nullptr) throw std::exception("Trying initialize multiple RHI drivers!"); s_instance = this; }
        virtual ~RHIDriver() = 0;
        virtual APIType GetAPI() const = 0;
        virtual RHIQueueSet* GetQueues() const = 0;
        virtual DriverMemoryInfo GetMemoryInfo() const = 0;
        virtual std::string GetDriverHeader() const = 0;
        virtual size_t GetBufferOffsetAlignment(BufferUsage usage) const = 0;
        virtual BuiltInResources* GetBuiltInResources() = 0;
        virtual Utilities::PropertyBlock* GetResourceState() = 0;
        
        virtual RHIAccelerationStructureRef CreateAccelerationStructure(const char* name) = 0;
        virtual RHITextureBindArrayRef CreateTextureBindArray(size_t capacity) = 0;
        virtual RHIBufferBindArrayRef CreateBufferBindArray(size_t capacity) = 0;
        virtual RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name) = 0;
        virtual RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name) = 0;
        virtual RHIShaderScope CreateShader(void* base, PK::Assets::Shader::PKShaderVariant* pVariant, const char* name) = 0;
        virtual RHIWindowScope CreateWindowScope(const WindowDescriptor& descriptor) = 0;

        virtual void SetBuffer(Utilities::NameID name, RHIBuffer* buffer, const IndexRange& range) = 0;
        virtual void SetTexture(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range) = 0;
        virtual void SetBufferArray(Utilities::NameID name, RHIBufferBindArray* bufferArray) = 0;
        virtual void SetTextureArray(Utilities::NameID name, RHITextureBindArray* textureArray) = 0;
        virtual void SetImage(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range) = 0;
        virtual void SetSampler(Utilities::NameID name, const SamplerDescriptor& sampler) = 0;
        virtual void SetAccelerationStructure(Utilities::NameID name, RHIAccelerationStructure* structure) = 0;
        virtual void SetConstant(Utilities::NameID name, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(Utilities::NameID name, bool value) = 0;

        virtual void WaitForIdle() const = 0;
        virtual void GC() = 0;

        static inline RHIDriver* Get() { return s_instance; }

        template<typename Child>
        static inline Child* GetNative()
        {
            static_assert(std::is_base_of<RHIDriver, Child>::value, "Child doesn't derive from base!");
            return static_cast<Child*>(s_instance);
        }
        
        protected: inline static RHIDriver* s_instance = nullptr;
    };
}