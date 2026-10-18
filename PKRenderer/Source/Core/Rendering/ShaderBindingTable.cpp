#include "PrecompiledHeader.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/RenderGraphExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "ShaderBindingTable.h"

namespace PK
{
    void ShaderBindingTable::Validate(RenderGraphExt graph, QueueType queue, ShaderAsset* shader, int32_t variantIndex)
    {
        if (variantIndex == -1)
        {
            variantIndex = shader->GetRHIIndex(graph->GetResourceState());
        }

        auto newHash = shader->GetAssetHash();

        if (pipelineHash != newHash || currentVariantIndex != (uint32_t)variantIndex)
        {
            pipelineHash = newHash;
            currentVariantIndex = (uint32_t)variantIndex;
            tableInfo = shader->GetRHI(currentVariantIndex)->GetShaderBindingTableInfo();
            RHI::ValidateBuffer(buffer, tableInfo.totalTableSize, BufferUsage::Vram | BufferUsage::ShaderBindingTable, "ShaderBindingTable");
            graph.UploadBufferData(queue, buffer.get(), tableInfo.handleData);
        }
    }
}
