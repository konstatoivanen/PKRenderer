#pragma once
#include "Core/Rendering/ShaderPropertyBlock.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct ConstantBuffer : public ShaderPropertyBlock
    {
        ConstantBuffer(const ShaderStructLayout& layout, const char* name);

        void FlushBuffer(RHICommandBuffer* cmd);

        const ShaderStructLayout& GetLayout() const;
        const RHIBuffer* GetRHI() const;
        RHIBuffer* GetRHI();
        operator RHIBuffer* ();
        operator const RHIBuffer* () const;

    private:
        RHIBufferRef m_graphicsBuffer;
        ShaderStructLayout m_layout;
    };
}
