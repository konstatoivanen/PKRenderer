#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/RHI/Objects/Buffer.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)

namespace PK::Rendering::Objects
{
    typedef PK::Utilities::Ref<class ConstantBuffer> ConstantBufferRef;

    class ConstantBuffer : public ShaderPropertyBlock
    {
    public:
        ConstantBuffer(const RHI::BufferLayout& layout, const char* name);

        void FlushBuffer(RHI::Objects::CommandBuffer* cmd);

        const RHI::BufferLayout& GetLayout() const { return m_layout; }
        const RHI::Objects::Buffer* GetBuffer() const { return m_graphicsBuffer.get(); }
        RHI::Objects::Buffer* GetBuffer() { return m_graphicsBuffer.get(); }

        operator RHI::Objects::Buffer* () { return m_graphicsBuffer.get(); }
        operator const RHI::Objects::Buffer* () { return m_graphicsBuffer.get(); }

    private:
        RHI::Objects::BufferRef m_graphicsBuffer;
        RHI::BufferLayout m_layout;
    };
}