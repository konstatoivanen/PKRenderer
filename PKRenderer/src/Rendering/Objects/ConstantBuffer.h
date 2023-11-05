#pragma once
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    typedef PK::Utilities::Ref<class ConstantBuffer> ConstantBufferRef;

    class ConstantBuffer : public ShaderPropertyBlock
    {
        public:
            ConstantBuffer(const RHI::BufferLayout& layout, const char* name);
            
            inline void FlushBuffer(RHI::QueueType queue)
            {
                RHI::GraphicsAPI::GetQueues()->GetCommandBuffer(queue)->UploadBufferData(m_graphicsBuffer.get(), m_buffer, 0ull, m_graphicsBuffer->GetCapacity());
            }

            const RHI::Objects::Buffer* GetBuffer() const { return m_graphicsBuffer.get(); }
            RHI::Objects::Buffer* GetBuffer() { return m_graphicsBuffer.get(); }

            operator RHI::Objects::Buffer* () { return m_graphicsBuffer.get(); }
            operator const RHI::Objects::Buffer* () { return m_graphicsBuffer.get(); }

        private:
            RHI::Objects::BufferRef m_graphicsBuffer;
    };
}