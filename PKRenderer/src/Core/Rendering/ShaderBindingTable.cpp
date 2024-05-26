#include "PrecompiledHeader.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "ShaderBindingTable.h"

namespace PK
{
    void ShaderBindingTable::Validate(CommandBufferExt cmd, ShaderAsset* shader)
    {
        // @TODO parameterize this
        auto selector = shader->GetRHISelector();
        selector.SetKeywordsFrom(*RHI::GetDriver()->GetResourceState());
        auto newVariantIndex = selector.GetIndex();
        auto newHash = shader->GetAssetHash();

        if (pipelineHash == newHash && variantIndex == newVariantIndex)
        {
            return;
        }

        pipelineHash = newHash;
        variantIndex = newVariantIndex;
        tableInfo = shader->GetRHI(newVariantIndex)->GetShaderBindingTableInfo();
        RHI::ValidateBuffer(buffer, tableInfo.totalTableSize, BufferUsage::DefaultShaderBindingTable, "ShaderBindingTable");
        cmd.UploadBufferData(buffer.get(), tableInfo.handleData);
    }
}