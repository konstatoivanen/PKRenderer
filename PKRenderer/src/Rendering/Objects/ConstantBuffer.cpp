#include "PrecompiledHeader.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "ConstantBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    ConstantBuffer::ConstantBuffer(const BufferLayout& layout, const char* name) :
        ShaderPropertyBlock(layout.GetStride()),
        m_graphicsBuffer(Buffer::Create(layout, BufferUsage::DefaultConstant, name))
    {
        ReserveLayout(layout);
        FreezeLayout();
    }

    void ConstantBuffer::FlushBuffer(RHI::Objects::CommandBuffer* cmd)
    {
        cmd->UpdateBuffer(m_graphicsBuffer.get(), 0ull, m_graphicsBuffer->GetCapacity(), m_buffer);
    }
}