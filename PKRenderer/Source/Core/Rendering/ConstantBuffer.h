#pragma once
#include "Core/Rendering/ShaderPropertyBlock.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct ConstantBuffer : public ShaderPropertyBlock
    {
        ConstantBuffer(const BufferLayout& layout, const char* name);

        void FlushBuffer(RHICommandBuffer* cmd);

        const BufferLayout& GetLayout() const;
        const RHIBuffer* GetRHI() const;
        RHIBuffer* GetRHI();
        operator RHIBuffer* ();
        operator const RHIBuffer* () const;

    private:
        RHIBufferRef m_graphicsBuffer;
        BufferLayout m_layout;
    };
}
