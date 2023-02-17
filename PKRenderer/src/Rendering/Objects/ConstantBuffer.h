#pragma once
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    class ConstantBuffer : public ShaderPropertyBlock
    {
        public:
            ConstantBuffer(const Structs::BufferLayout& layout, const char* name);
            inline void FlushBuffer() { GraphicsAPI::GetCommandBuffer(Structs::QueueType::Graphics)->UploadBufferData(m_graphicsBuffer.get(), m_buffer, 0ull, m_graphicsBuffer->GetCapacity()); }

            const Buffer* GetBuffer() const { return m_graphicsBuffer.get(); }
            Buffer* GetBuffer() { return m_graphicsBuffer.get(); }

            operator Buffer* () { return m_graphicsBuffer.get(); }
            operator const Buffer* () { return m_graphicsBuffer.get(); }

        private:
            Utilities::Ref<Buffer> m_graphicsBuffer;
    };
}