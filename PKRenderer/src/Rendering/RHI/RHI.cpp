#include "PrecompiledHeader.h"
#include "Rendering/RHI/Driver.h"
#include "Rendering/RHI/Objects/AccelerationStructure.h"
#include "Rendering/RHI/Objects/BindArray.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Objects/QueueSet.h"
#include "Rendering/RHI/Objects/Window.h"
#include "RHI.h"

namespace PK::Rendering::RHI
{
    namespace Objects
    {
        AccelerationStructure::~AccelerationStructure() = default;
        template<> BindArray<Texture>::~BindArray() = default;
        template<> BindArray<Buffer>::~BindArray() = default;
        Buffer::~Buffer() = default;
        QueueSet::~QueueSet() = default;
        Window::~Window() = default;
        Texture::~Texture() = default;
    }

    using namespace PK::Rendering::RHI::Objects;

    Driver* RHIGetDriver() { return Driver::Get(); }
    APIType RHIGetActiveAPI() { return Driver::Get()->GetAPI(); }
    RHI::Objects::QueueSet* RHIGetQueues() { return Driver::Get()->GetQueues(); }
    RHI::Objects::CommandBuffer* RHIGetCommandBuffer(QueueType queue) { return Driver::Get()->GetQueues()->GetCommandBuffer(queue); }
    DriverMemoryInfo RHIGetMemoryInfo() { return Driver::Get()->GetMemoryInfo(); }
    size_t RHIGetBufferOffsetAlignment(BufferUsage usage) { return Driver::Get()->GetBufferOffsetAlignment(usage); }
    const BuiltInResources* RHIGetBuiltInResources() { return Driver::Get()->builtInResources; }
    void RHIGC() { Driver::Get()->GC(); }

    BufferRef RHICreateBuffer(size_t size, BufferUsage usage, const char* name) { return Driver::Get()->CreateBuffer(size, usage, name); }
    TextureRef RHICreateTexture(const TextureDescriptor& descriptor, const char* name) { return Driver::Get()->CreateTexture(descriptor, name); }
    AccelerationStructureRef RHICreateAccelerationStructure(const char* name) { return Driver::Get()->CreateAccelerationStructure(name); }
    WindowScope RHICreateWindow(const WindowProperties& properties) { return Driver::Get()->CreateRHIWindow(properties); }
    template<> TextureBindArrayRef RHICreateBindArray(size_t capacity) { return Driver::Get()->CreateTextureBindArray(capacity); }
    template<> BufferBindArrayRef RHICreateBindArray(size_t capacity) { return Driver::Get()->CreateBufferBindArray(capacity); }

    bool RHIValidateTexture(RHI::Objects::TextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name)
    {
        if (!inoutTexture)
        {
            inoutTexture = RHICreateTexture(descriptor, name);
            return true;
        }
        else
        {
            return inoutTexture->Validate(descriptor);
        }
    }

    bool RHIValidateBuffer(RHI::Objects::BufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name)
    {
        if (!inoutBuffer)
        {
            inoutBuffer = RHICreateBuffer(size, usage, name);
            return true;
        }
        else
        {
            return inoutBuffer->Validate(size);
        }
    }

    void RHISetBuffer(Utilities::NameID name, Buffer* buffer, const IndexRange& range) { Driver::Get()->SetBuffer(name, buffer, range); }
    void RHISetBuffer(Utilities::NameID name, Buffer* buffer) { Driver::Get()->SetBuffer(name, buffer); }
    void RHISetTexture(Utilities::NameID name, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetTexture(name, texture, range); }
    void RHISetTexture(Utilities::NameID name, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetTexture(name, texture, level, layer); }
    void RHISetTexture(Utilities::NameID name, Texture* texture) { Driver::Get()->SetTexture(name, texture); }
    void RHISetImage(Utilities::NameID name, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetImage(name, texture, range); }
    void RHISetImage(Utilities::NameID name, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetImage(name, texture, level, layer); }
    void RHISetImage(Utilities::NameID name, Texture* texture) { Driver::Get()->SetImage(name, texture); }
    void RHISetSampler(Utilities::NameID name, const SamplerDescriptor& sampler) { Driver::Get()->SetSampler(name, sampler); }
    void RHISetAccelerationStructure(Utilities::NameID name, AccelerationStructure* structure) { Driver::Get()->SetAccelerationStructure(name, structure); }
    void RHISetBufferArray(Utilities::NameID name, BindArray<Buffer>* bufferArray) { Driver::Get()->SetBufferArray(name, bufferArray); }
    void RHISetTextureArray(Utilities::NameID name, BindArray<Texture>* textureArray) { Driver::Get()->SetTextureArray(name, textureArray); }
    void RHISetConstant(Utilities::NameID name, const void* data, uint32_t size) { Driver::Get()->SetConstant(name, data, size); }
    void RHISetKeyword(Utilities::NameID name, bool value) { Driver::Get()->SetKeyword(name, value); }
}
