#include "PrecompiledHeader.h"
#include "Core/RHI/RHInterfaces.h"
#include "ConstantBuffer.h"

namespace PK
{
    ConstantBuffer::ConstantBuffer(const BufferLayout& layout, const char* name) :
        ShaderPropertyBlock(layout.GetStride(), layout.size()),
        m_graphicsBuffer(RHI::CreateBuffer(layout.GetAlignedStride(), BufferUsage::DefaultConstant, name)),
        m_layout(layout)
    {
        ReserveLayout(layout);
        FreezeLayout();
    }

    void ConstantBuffer::FlushBuffer(RHICommandBuffer* cmd)
    {
        cmd->UpdateBuffer(m_graphicsBuffer.get(), 0ull, m_graphicsBuffer->GetSize(), m_buffer);
    }

    const BufferLayout& ConstantBuffer::GetLayout() const { return m_layout; }
    const RHIBuffer* ConstantBuffer::GetRHI() const { return m_graphicsBuffer.get(); }
    RHIBuffer* ConstantBuffer::GetRHI() { return m_graphicsBuffer.get(); }
    ConstantBuffer::operator RHIBuffer* () { return m_graphicsBuffer.get(); }
    ConstantBuffer::operator const RHIBuffer* () const { return m_graphicsBuffer.get(); }
}