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
            inline void FlushBuffer() { m_buffer->SetData(data(), 0, size()); }

            operator Buffer* () { return m_buffer.get(); }
            operator const Buffer* () { return m_buffer.get(); }

        private:
            Ref<Buffer> m_buffer;
    };
}