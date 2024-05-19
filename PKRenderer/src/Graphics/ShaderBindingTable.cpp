#include "PrecompiledHeader.h"
#include "Graphics/RHI/RHIDriver.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/Shader.h"
#include "ShaderBindingTable.h"

namespace PK::Graphics
{
    using namespace PK::Graphics::RHI;

    void ShaderBindingTable::Validate(CommandBufferExt cmd, Shader* shader)
    {
        // @TODO parameterize this
        auto selector = shader->GetRHISelector();
        selector.SetKeywordsFrom(*RHIGetDriver()->GetResourceState());
        auto newVariantIndex = selector.GetIndex();
        auto newHash = shader->GetAssetHash();

        if (pipelineHash == newHash && variantIndex == newVariantIndex)
        {
            return;
        }

        pipelineHash = newHash;
        variantIndex = newVariantIndex;
        tableInfo = shader->GetRHI(newVariantIndex)->GetShaderBindingTableInfo();
        RHIValidateBuffer(buffer, tableInfo.totalTableSize, BufferUsage::DefaultShaderBindingTable, "ShaderBindingTable");
        cmd.UploadBufferData(buffer.get(), tableInfo.handleData);
    }
}