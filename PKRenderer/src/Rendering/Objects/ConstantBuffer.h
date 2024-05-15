#pragma once
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::Objects
{
    typedef PK::Utilities::Ref<class ConstantBuffer> ConstantBufferRef;

    class ConstantBuffer : public ShaderPropertyBlock
    {
    public:
        ConstantBuffer(const RHI::BufferLayout& layout, const char* name);

        void FlushBuffer(RHI::Objects::CommandBuffer* cmd);

        const RHI::BufferLayout& GetLayout() const { return m_layout; }
        const RHI::Objects::Buffer* GetRHI() const { return m_graphicsBuffer.get(); }
        RHI::Objects::Buffer* GetRHI() { return m_graphicsBuffer.get(); }

        operator RHI::Objects::Buffer* () { return m_graphicsBuffer.get(); }
        operator const RHI::Objects::Buffer* () { return m_graphicsBuffer.get(); }

    private:
        RHI::Objects::BufferRef m_graphicsBuffer;
        RHI::BufferLayout m_layout;
    };
}