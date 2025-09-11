#include "PrecompiledHeader.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "ShaderBindingTable.h"

namespace PK
{
    void ShaderBindingTable::Validate(CommandBufferExt cmd, ShaderAsset* shader, int32_t variantIndex)
    {
        if (variantIndex == -1)
        {
            variantIndex = shader->GetRHIIndex(RHI::GetDriver()->GetResourceState());
        }

        auto newHash = shader->GetAssetHash();

        if (pipelineHash == newHash && currentVariantIndex == (uint32_t)variantIndex)
        {
            return;
        }

        pipelineHash = newHash;
        currentVariantIndex = (uint32_t)variantIndex;
        tableInfo = shader->GetRHI(currentVariantIndex)->GetShaderBindingTableInfo();
        RHI::ValidateBuffer(buffer, tableInfo.totalTableSize, BufferUsage::DefaultShaderBindingTable, "ShaderBindingTable");
        cmd.UploadBufferData(buffer.get(), tableInfo.handleData);
    }
}
