#pragma once
#include "Core/Rendering/ShaderPropertyBlock.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct ConstantBuffer : public ShaderPropertyBlock
    {
        ConstantBuffer(const ShaderPropertyLayout& layout, const char* name);
        ConstantBuffer(ShaderProperty* elements, size_t count, const char* name);
        ConstantBuffer(std::initializer_list<ShaderProperty> elements, const char* name);

        void FlushBuffer(RHICommandBuffer* cmd);

        const RHIBuffer* GetRHI() const;
        RHIBuffer* GetRHI();
        operator RHIBuffer* ();
        operator const RHIBuffer* () const;

    private:
        RHIBufferRef m_rhiBuffer;
    };

    struct ConstantBufferTransient : public ShaderPropertyWriter
    {
        ConstantBufferTransient();
        void BeginWrite(const ShaderPropertyLayout* layout, void* memory, const char* name);
        void EndWrite(RHICommandBuffer* cmd);
        constexpr uint64_t GetHash() const { return m_layoutHash; }
        const RHIBuffer* GetRHI() const;
        RHIBuffer* GetRHI();
        operator RHIBuffer* ();
        operator const RHIBuffer* () const;

    private:
        RHIBufferRef m_rhiBuffer;
        uint64_t m_layoutHash;
    };
}
