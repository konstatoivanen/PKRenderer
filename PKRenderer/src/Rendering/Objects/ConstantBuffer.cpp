#include "PrecompiledHeader.h"
#include "ConstantBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace Structs;

    ConstantBuffer::ConstantBuffer(const BufferLayout& layout, const char* name) : 
        ShaderPropertyBlock(nullptr, 0ull),
        m_graphicsBuffer(Buffer::CreateConstant(layout, BufferUsage::PersistentStage, name))
    {
        SetForeign(m_graphicsBuffer->BeginWrite(0ull, m_graphicsBuffer->GetCapacity()), m_graphicsBuffer->GetCapacity());
        ReserveLayout(layout);
        FreezeLayout();
    }
}