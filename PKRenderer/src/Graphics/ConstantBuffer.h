#pragma once
#include "Graphics/ShaderPropertyBlock.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    struct ConstantBuffer : public ShaderPropertyBlock
    {
        ConstantBuffer(const RHI::BufferLayout& layout, const char* name);

        void FlushBuffer(CommandBuffer* cmd);

        const RHI::BufferLayout& GetLayout() const;
        const Buffer* GetRHI() const;
        Buffer* GetRHI();
        operator Buffer* ();
        operator const Buffer* () const;

    private:
        BufferRef m_graphicsBuffer;
        RHI::BufferLayout m_layout;
    };
}