#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/NameID.h"
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

        void SetBuffer(Utilities::NameID name, RHI::Objects::Buffer* buffer, const IndexRange& range);
        void SetBuffer(Utilities::NameID name, RHI::Objects::Buffer* buffer);
        void SetTexture(Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetTexture(Utilities::NameID name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(Utilities::NameID name, RHI::Objects::Texture* texture);
        void SetImage(Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range);
        void SetImage(Utilities::NameID name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(Utilities::NameID name, RHI::Objects::Texture* texture);
        void SetSampler(Utilities::NameID name, const SamplerDescriptor& sampler);
        void SetAccelerationStructure(Utilities::NameID name, RHI::Objects::AccelerationStructure* structure);
        void SetBufferArray(Utilities::NameID name, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray);
        void SetTextureArray(Utilities::NameID name, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray);
        void SetConstant(Utilities::NameID name, const void* data, uint32_t size);
        void SetKeyword(Utilities::NameID name, bool value);

        template<typename T>
        void SetConstant(Utilities::NameID name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
    }
}