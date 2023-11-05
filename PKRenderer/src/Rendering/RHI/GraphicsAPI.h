#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/FenceRef.h"
#include "Rendering/RHI/Layout.h"
#include "Rendering/RHI/Objects/AccelerationStructure.h"
#include "Rendering/RHI/Objects/BindArray.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Objects/QueueSet.h"
#include "Rendering/RHI/Driver.h"

namespace PK::Rendering::RHI
{
    namespace GraphicsAPI
    {
        inline Driver* GetDriver() { return Driver::Get(); }
        inline APIType GetActiveAPI() { return Driver::Get()->GetAPI(); }
        inline RHI::Objects::QueueSet* GetQueues() { return Driver::Get()->GetQueues(); }
        inline DriverMemoryInfo GetMemoryInfo() { return Driver::Get()->GetMemoryInfo(); }
        inline size_t GetBufferOffsetAlignment(BufferUsage usage) { return Driver::Get()->GetBufferOffsetAlignment(usage); }
        inline const BuiltInResources* GetBuiltInResources() { return Driver::Get()->builtInResources; }
        inline void GC() { Driver::Get()->GC(); }

        inline void SetBuffer(uint32_t nameHashId, RHI::Objects::Buffer* buffer, const IndexRange& range) { Driver::Get()->SetBuffer(nameHashId, buffer, range); }
        inline void SetBuffer(uint32_t nameHashId, RHI::Objects::Buffer* buffer) { Driver::Get()->SetBuffer(nameHashId, buffer); }
        inline void SetBuffer(const char* name, RHI::Objects::Buffer* buffer, const IndexRange& range) { Driver::Get()->SetBuffer(name, buffer, range); }
        inline void SetBuffer(const char* name, RHI::Objects::Buffer* buffer) { Driver::Get()->SetBuffer(name, buffer); }
        inline void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture, const TextureViewRange& range) { Driver::Get()->SetTexture(nameHashId, texture, range); }
        inline void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetTexture(nameHashId, texture, level, layer); }
        inline void SetTexture(uint32_t nameHashId, RHI::Objects::Texture* texture) { Driver::Get()->SetTexture(nameHashId, texture); }
        inline void SetTexture(const char* name, RHI::Objects::Texture* texture, const TextureViewRange& range) { Driver::Get()->SetTexture(name, texture, range); }
        inline void SetTexture(const char* name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetTexture(name, texture, level, layer); }
        inline void SetTexture(const char* name, RHI::Objects::Texture* texture) { Driver::Get()->SetTexture(name, texture); }
        inline void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture, const TextureViewRange& range) { Driver::Get()->SetImage(nameHashId, texture, range); }
        inline void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetImage(nameHashId, texture, level, layer); }
        inline void SetImage(uint32_t nameHashId, RHI::Objects::Texture* texture) { Driver::Get()->SetImage(nameHashId, texture); }
        inline void SetImage(const char* name, RHI::Objects::Texture* texture, const TextureViewRange& range) { Driver::Get()->SetImage(name, texture, range); }
        inline void SetImage(const char* name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer) { Driver::Get()->SetImage(name, texture, level, layer); }
        inline void SetImage(const char* name, RHI::Objects::Texture* texture) { Driver::Get()->SetImage(name, texture); }
        inline void SetSampler(uint32_t nameHashId, const SamplerDescriptor& sampler) { Driver::Get()->SetSampler(nameHashId, sampler); }
        inline void SetSampler(const char* name, const SamplerDescriptor& sampler) { Driver::Get()->SetSampler(name, sampler); }
        inline void SetAccelerationStructure(uint32_t nameHashId, RHI::Objects::AccelerationStructure* structure) { Driver::Get()->SetAccelerationStructure(nameHashId, structure); }
        inline void SetAccelerationStructure(const char* name, RHI::Objects::AccelerationStructure* structure) { Driver::Get()->SetAccelerationStructure(name, structure); }
        inline void SetBufferArray(uint32_t nameHashId, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray) { Driver::Get()->SetBufferArray(nameHashId, bufferArray); }
        inline void SetBufferArray(const char* name, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray) { Driver::Get()->SetBufferArray(name, bufferArray); }
        inline void SetTextureArray(uint32_t nameHashId, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray) { Driver::Get()->SetTextureArray(nameHashId, textureArray); }
        inline void SetTextureArray(const char* name, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray) { Driver::Get()->SetTextureArray(name, textureArray); }
        inline void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) { Driver::Get()->SetConstant(nameHashId, data, size); }
        inline void SetConstant(const char* name, const void* data, uint32_t size) { Driver::Get()->SetConstant(name, data, size); }
        inline void SetKeyword(uint32_t nameHashId, bool value) { Driver::Get()->SetKeyword(nameHashId, value); }
        inline void SetKeyword(const char* name, bool value) { Driver::Get()->SetKeyword(name, value); }

        template<typename T>
        void SetConstant(uint32_t nameHashId, const T& value) { SetConstant(nameHashId, &value, (uint32_t)sizeof(T)); }

        template<typename T>
        void SetConstant(const char* name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
    }
}