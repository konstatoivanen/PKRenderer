#pragma once
#include "Utilities/ForwardDeclareUtility.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/Objects/BindArray.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct Driver)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct Window)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct BuiltInResources)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct QueueSet)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Buffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Texture)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class AccelerationStructure)

namespace PK::Rendering::RHI
{
    namespace GraphicsAPI
    {
        Driver* GetDriver();
        APIType GetActiveAPI();
        RHI::Objects::QueueSet* GetQueues();
        RHI::Objects::CommandBuffer* GetCommandBuffer(QueueType queue);
        DriverMemoryInfo GetMemoryInfo();
        size_t GetBufferOffsetAlignment(BufferUsage usage);
        const BuiltInResources* GetBuiltInResources();
        void GC();

        void SetBuffer(uint32_t nameHashId, RHI::Objects::Buffer* buffer, const IndexRange& range);
        void SetBuffer(uint32_t nameHashId, RHI::Objects::Buffer* buffer);
        void SetBuffer(const char* name, RHI::Objects::Buffer* buffer, const IndexRange& range);
        void SetBuffer(const char* name, RHI::Objects::Buffer* buffer);
        void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture);
        void SetTexture(const char* name, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetTexture(const char* name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(const char* name, RHI::Objects::Texture* texture);
        void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture);
        void SetImage(const char* name, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetImage(const char* name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(const char* name, RHI::Objects::Texture* texture);
        void SetSampler(uint32_t nameHashId, const SamplerDescriptor& sampler);
        void SetSampler(const char* name, const SamplerDescriptor& sampler);
        void SetAccelerationStructure(uint32_t nameHashId, RHI::Objects::AccelerationStructure* structure);
        void SetAccelerationStructure(const char* name, RHI::Objects::AccelerationStructure* structure);
        void SetBufferArray(uint32_t nameHashId, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray);
        void SetBufferArray(const char* name, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray);
        void SetTextureArray(uint32_t nameHashId, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray);
        void SetTextureArray(const char* name, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray);
        void SetConstant(uint32_t nameHashId, const void* data, uint32_t size);
        void SetConstant(const char* name, const void* data, uint32_t size);
        void SetKeyword(uint32_t nameHashId, bool value);
        void SetKeyword(const char* name, bool value);

        template<typename T>
        void SetConstant(uint32_t nameHashId, const T& value) { SetConstant(nameHashId, &value, (uint32_t)sizeof(T)); }

        template<typename T>
        void SetConstant(const char* name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
    }
}