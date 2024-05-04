#include "PrecompiledHeader.h"
#include "Rendering/RHI/Driver.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Objects/QueueSet.h"
#include "GraphicsAPI.h"

namespace PK::Rendering::RHI
{
    using namespace PK::Rendering::RHI::Objects;

    Driver* GraphicsAPI::GetDriver() { return Driver::Get(); }
    APIType GraphicsAPI::GetActiveAPI() { return Driver::Get()->GetAPI(); }
    RHI::Objects::QueueSet* GraphicsAPI::GetQueues() { return Driver::Get()->GetQueues(); }
    RHI::Objects::CommandBuffer* GraphicsAPI::GetCommandBuffer(QueueType queue) { return Driver::Get()->GetQueues()->GetCommandBuffer(queue); }
    DriverMemoryInfo GraphicsAPI::GetMemoryInfo() { return Driver::Get()->GetMemoryInfo(); }
    size_t GraphicsAPI::GetBufferOffsetAlignment(BufferUsage usage) { return Driver::Get()->GetBufferOffsetAlignment(usage); }
    const BuiltInResources* GraphicsAPI::GetBuiltInResources() { return Driver::Get()->builtInResources; }
    void GraphicsAPI::GC() { Driver::Get()->GC(); }

    void GraphicsAPI::SetBuffer(Utilities::NameID name, Buffer* buffer, const IndexRange& range) { Driver::Get()->SetBuffer(name, buffer, range); }
    void GraphicsAPI::SetBuffer(Utilities::NameID name, Buffer* buffer) { Driver::Get()->SetBuffer(name, buffer); }
    void GraphicsAPI::SetTexture(Utilities::NameID name, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetTexture(name, texture, range); }
    void GraphicsAPI::SetTexture(Utilities::NameID name, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetTexture(name, texture, level, layer); }
    void GraphicsAPI::SetTexture(Utilities::NameID name, Texture* texture) { Driver::Get()->SetTexture(name, texture); }
    void GraphicsAPI::SetImage(Utilities::NameID name, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetImage(name, texture, range); }
    void GraphicsAPI::SetImage(Utilities::NameID name, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetImage(name, texture, level, layer); }
    void GraphicsAPI::SetImage(Utilities::NameID name, Texture* texture) { Driver::Get()->SetImage(name, texture); }
    void GraphicsAPI::SetSampler(Utilities::NameID name, const SamplerDescriptor& sampler) { Driver::Get()->SetSampler(name, sampler); }
    void GraphicsAPI::SetAccelerationStructure(Utilities::NameID name, AccelerationStructure* structure) { Driver::Get()->SetAccelerationStructure(name, structure); }
    void GraphicsAPI::SetBufferArray(Utilities::NameID name, BindArray<Buffer>* bufferArray) { Driver::Get()->SetBufferArray(name, bufferArray); }
    void GraphicsAPI::SetTextureArray(Utilities::NameID name, BindArray<Texture>* textureArray) { Driver::Get()->SetTextureArray(name, textureArray); }
    void GraphicsAPI::SetConstant(Utilities::NameID name, const void* data, uint32_t size) { Driver::Get()->SetConstant(name, data, size); }
    void GraphicsAPI::SetKeyword(Utilities::NameID name, bool value) { Driver::Get()->SetKeyword(name, value); }
}
