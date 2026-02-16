#include "PrecompiledHeader.h"
#include "Core/RHI/RHInterfaces.h"
#include "ConstantBuffer.h"

namespace PK
{
    ConstantBuffer::ConstantBuffer(const ShaderPropertyLayout& layout, const char* name) : 
        ShaderPropertyBlock(layout),
        m_rhiBuffer(RHI::CreateBuffer(GetLayout().GetStridePadded(), BufferUsage::DefaultConstant, name))
    {
    }

    ConstantBuffer::ConstantBuffer(ShaderProperty* elements, size_t count, const char* name) :
        ShaderPropertyBlock(elements, count),
        m_rhiBuffer(RHI::CreateBuffer(GetLayout().GetStridePadded(), BufferUsage::DefaultConstant, name))
    {
    }

    ConstantBuffer::ConstantBuffer(std::initializer_list<ShaderProperty> elements, const char* name) :
        ShaderPropertyBlock(elements),
        m_rhiBuffer(RHI::CreateBuffer(GetLayout().GetStridePadded(), BufferUsage::DefaultConstant, name))
    {
    }

    void ConstantBuffer::FlushBuffer(RHICommandBuffer* cmd)
    {
        cmd->UpdateBuffer(m_rhiBuffer.get(), 0ull, m_rhiBuffer->GetSize(), GetData());
    }

    const RHIBuffer* ConstantBuffer::GetRHI() const { return m_rhiBuffer.get(); }
    RHIBuffer* ConstantBuffer::GetRHI() { return m_rhiBuffer.get(); }
    ConstantBuffer::operator RHIBuffer* () { return m_rhiBuffer.get(); }
    ConstantBuffer::operator const RHIBuffer* () const { return m_rhiBuffer.get(); }


    ConstantBufferTransient::ConstantBufferTransient() : m_rhiBuffer(nullptr), m_layoutHash(0ull) {}

    void ConstantBufferTransient::BeginWrite(const ShaderPropertyLayout* layout, void* memory, const char* name)
    {
        m_layoutHash = layout->GetHash();
        ShaderPropertyWriter::BeginWrite(layout, memory);
        RHI::ValidateBuffer(m_rhiBuffer, layout->GetStride(), BufferUsage::DefaultConstant, name);
    }

    void ConstantBufferTransient::EndWrite(RHICommandBuffer* cmd)
    {
        cmd->UpdateBuffer(m_rhiBuffer.get(), 0ull, m_rhiBuffer->GetSize(), ShaderPropertyWriter::EndWrite());
    }

    const RHIBuffer* ConstantBufferTransient::GetRHI() const { return m_rhiBuffer.get(); }
    RHIBuffer* ConstantBufferTransient::GetRHI() { return m_rhiBuffer.get(); }
    ConstantBufferTransient::operator RHIBuffer* () { return m_rhiBuffer.get(); }
    ConstantBufferTransient::operator const RHIBuffer* () const { return m_rhiBuffer.get(); }
}
