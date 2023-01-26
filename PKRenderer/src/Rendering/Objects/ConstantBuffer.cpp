#include "PrecompiledHeader.h"
#include "ConstantBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace Structs;

    ConstantBuffer::ConstantBuffer(const BufferLayout& layout, const char* name) : 
        ShaderPropertyBlock(layout.GetStride()),
        m_graphicsBuffer(Buffer::Create(layout, BufferUsage::DefaultConstant | BufferUsage::PersistentStage, name))
    {
        ReserveLayout(layout);
        FreezeLayout();
    }
}