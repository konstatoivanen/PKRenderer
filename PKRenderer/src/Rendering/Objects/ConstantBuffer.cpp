#include "PrecompiledHeader.h"
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
}