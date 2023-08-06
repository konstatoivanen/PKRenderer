#include "PrecompiledHeader.h"
#include "ShaderBindingTable.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Rendering::Structs;

    void ShaderBindingTable::Validate(CommandBuffer* cmdUpload, CommandBuffer* cmdBind, Shader* shader)
    {
        // @TODO parameterize this
        auto selector = shader->GetVariantSelector();
        selector.SetKeywordsFrom(GraphicsAPI::GetActiveDriver()->globalResources);
        auto newVariantIndex = selector.GetIndex();
        auto newHash = shader->GetAssetHash();

        if (pipelineHash == newHash && variantIndex == newVariantIndex)
        {
            return;
        }

        pipelineHash = newHash;
        variantIndex = newVariantIndex;
        tableInfo = shader->GetVariant(newVariantIndex)->GetShaderBindingTableInfo();
        auto tableUintCount = tableInfo.totalTableSize / sizeof(uint32_t);

        if (buffer == nullptr)
        {
            buffer = Buffer::Create(ElementType::Uint, tableUintCount, BufferUsage::DefaultShaderBindingTable, "ShaderBindingTable");
        }
        else
        {
            buffer->Validate(tableUintCount);
        }

        cmdUpload->UploadBufferData(buffer.get(), tableInfo.handleData);

        cmdBind->SetShaderBindingTable(RayTracingShaderGroup::RayGeneration,
            buffer.get(),
            tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::RayGeneration],
            tableInfo.handleSizeAligned,
            tableInfo.handleSizeAligned);

        cmdBind->SetShaderBindingTable(RayTracingShaderGroup::Miss,
            buffer.get(),
            tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Miss],
            tableInfo.handleSizeAligned,
            tableInfo.handleSizeAligned);

        cmdBind->SetShaderBindingTable(RayTracingShaderGroup::Hit,
            buffer.get(),
            tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Hit],
            tableInfo.handleSizeAligned,
            tableInfo.handleSizeAligned);
    }
}