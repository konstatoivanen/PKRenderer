#pragma once
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/Objects/Buffer.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;

    class ConstantBuffer : public ShaderPropertyBlock
    {
        public:
            ConstantBuffer(const BufferLayout& layout);
            inline void FlushBuffer() { m_graphicsBuffer->EndWrite(); }

            const Buffer* GetBuffer() const { return m_graphicsBuffer.get(); }
            Buffer* GetBuffer() { return m_graphicsBuffer.get(); }

            operator Buffer* () { return m_graphicsBuffer.get(); }
            operator const Buffer* () { return m_graphicsBuffer.get(); }

        private:
            Ref<Buffer> m_graphicsBuffer;
    };
}