#include "PrecompiledHeader.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/RHI/RHIBuffer.h"
#include "ConstantBuffer.h"

namespace PK::Graphics
{
    using namespace PK::Graphics::RHI;

    ConstantBuffer::ConstantBuffer(const BufferLayout& layout, const char* name) :
        ShaderPropertyBlock(layout.GetStride(), layout.size()),
        m_graphicsBuffer(RHICreateBuffer(layout.GetAlignedStride(), BufferUsage::DefaultConstant, name)),
        m_layout(layout)
    {
        ReserveLayout(layout);
        FreezeLayout();
    }

    void ConstantBuffer::FlushBuffer(CommandBuffer* cmd)
    {
        cmd->UpdateBuffer(m_graphicsBuffer.get(), 0ull, m_graphicsBuffer->GetCapacity(), m_buffer);
    }

    const BufferLayout& ConstantBuffer::GetLayout() const { return m_layout; }
    const Buffer* ConstantBuffer::GetRHI() const { return m_graphicsBuffer.get(); }
    Buffer* ConstantBuffer::GetRHI() { return m_graphicsBuffer.get(); }
    ConstantBuffer::operator Buffer* () { return m_graphicsBuffer.get(); }
    ConstantBuffer::operator const Buffer* () const { return m_graphicsBuffer.get(); }
}