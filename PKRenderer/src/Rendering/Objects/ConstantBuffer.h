#pragma once
#include "Core/PropertyBlock.h"
#include "Rendering/Objects/Buffer.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;

    class ConstantBuffer : public PK::Core::PropertyBlock
    {
        public:
            ConstantBuffer(const BufferLayout& layout);
            inline void FlushBuffer() { m_graphicsBuffer->SetData(m_buffer, 0, m_capacity); }

            operator Buffer* () { return m_graphicsBuffer.get(); }
            operator const Buffer* () { return m_graphicsBuffer.get(); }

        private:
            Ref<Buffer> m_graphicsBuffer;
    };
}