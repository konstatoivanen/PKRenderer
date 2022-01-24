#include "PrecompiledHeader.h"
#include "ConstantBuffer.h"

namespace PK::Rendering::Objects
{
    ConstantBuffer::ConstantBuffer(const BufferLayout& layout) : 
        ShaderPropertyBlock(nullptr, 0ull),
        m_graphicsBuffer(Buffer::CreateConstant(layout, BufferUsage::PersistentStage))
    {
        SetForeign(m_graphicsBuffer->BeginWrite(0ull, m_graphicsBuffer->GetCapacity()), m_graphicsBuffer->GetCapacity());
        ReserveLayout(layout);
        FreezeLayout();
    }
}