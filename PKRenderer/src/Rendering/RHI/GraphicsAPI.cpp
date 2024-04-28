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

    void GraphicsAPI::SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range) { Driver::Get()->SetBuffer(nameHashId, buffer, range); }
    void GraphicsAPI::SetBuffer(uint32_t nameHashId, Buffer* buffer) { Driver::Get()->SetBuffer(nameHashId, buffer); }
    void GraphicsAPI::SetBuffer(const char* name, Buffer* buffer, const IndexRange& range) { Driver::Get()->SetBuffer(name, buffer, range); }
    void GraphicsAPI::SetBuffer(const char* name, Buffer* buffer) { Driver::Get()->SetBuffer(name, buffer); }
    void GraphicsAPI::SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetTexture(nameHashId, texture, range); }
    void GraphicsAPI::SetTexture(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetTexture(nameHashId, texture, level, layer); }
    void GraphicsAPI::SetTexture(uint32_t nameHashId, Texture* texture) { Driver::Get()->SetTexture(nameHashId, texture); }
    void GraphicsAPI::SetTexture(const char* name, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetTexture(name, texture, range); }
    void GraphicsAPI::SetTexture(const char* name, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetTexture(name, texture, level, layer); }
    void GraphicsAPI::SetTexture(const char* name, Texture* texture) { Driver::Get()->SetTexture(name, texture); }
    void GraphicsAPI::SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetImage(nameHashId, texture, range); }
    void GraphicsAPI::SetImage(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetImage(nameHashId, texture, level, layer); }
    void GraphicsAPI::SetImage(uint32_t nameHashId, Texture* texture) { Driver::Get()->SetImage(nameHashId, texture); }
    void GraphicsAPI::SetImage(const char* name, Texture* texture, const TextureViewRange& range) { Driver::Get()->SetImage(name, texture, range); }
    void GraphicsAPI::SetImage(const char* name, Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetImage(name, texture, level, layer); }
    void GraphicsAPI::SetImage(const char* name, Texture* texture) { Driver::Get()->SetImage(name, texture); }
    void GraphicsAPI::SetSampler(uint32_t nameHashId, const SamplerDescriptor& sampler) { Driver::Get()->SetSampler(nameHashId, sampler); }
    void GraphicsAPI::SetSampler(const char* name, const SamplerDescriptor& sampler) { Driver::Get()->SetSampler(name, sampler); }
    void GraphicsAPI::SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure) { Driver::Get()->SetAccelerationStructure(nameHashId, structure); }
    void GraphicsAPI::SetAccelerationStructure(const char* name, AccelerationStructure* structure) { Driver::Get()->SetAccelerationStructure(name, structure); }
    void GraphicsAPI::SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray) { Driver::Get()->SetBufferArray(nameHashId, bufferArray); }
    void GraphicsAPI::SetBufferArray(const char* name, BindArray<Buffer>* bufferArray) { Driver::Get()->SetBufferArray(name, bufferArray); }
    void GraphicsAPI::SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray) { Driver::Get()->SetTextureArray(nameHashId, textureArray); }
    void GraphicsAPI::SetTextureArray(const char* name, BindArray<Texture>* textureArray) { Driver::Get()->SetTextureArray(name, textureArray); }
    void GraphicsAPI::SetConstant(uint32_t nameHashId, const void* data, uint32_t size) { Driver::Get()->SetConstant(nameHashId, data, size); }
    void GraphicsAPI::SetConstant(const char* name, const void* data, uint32_t size) { Driver::Get()->SetConstant(name, data, size); }
    void GraphicsAPI::SetKeyword(uint32_t nameHashId, bool value) { Driver::Get()->SetKeyword(nameHashId, value); }
    void GraphicsAPI::SetKeyword(const char* name, bool value) { Driver::Get()->SetKeyword(name, value); }
}
