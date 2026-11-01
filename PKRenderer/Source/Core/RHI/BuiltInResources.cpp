#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "BuiltInResources.h"

PK::BuiltInResources::BuiltInResources()
{
    PK_LOG_RHI_FUNC();

    TextureDescriptor textureDesc{};
    textureDesc.resolution = { 2u, 2u, 1u };
    textureDesc.usage = TextureUsage::Default;

    BlackTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Black");
    WhiteTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.White");
    ErrorTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Error");
    TransparentTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Transparent");
   
    textureDesc.layers = 2;
    textureDesc.type = TextureType::Texture2DArray;
    BlackTexture2DArray = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2DArray.Black");

    const uint32_t blackData[4] = { 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u };
    const uint32_t blackArrayData[8] = { 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u };
    const uint32_t whiteData[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    const uint32_t errorData[4] = { 0xFFFF00FFu, 0xFF000000u, 0xFF000000u, 0xFFFF00FFu };
    const uint32_t transparentData[4] = { 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu };
    const uint32_t stageSize = sizeof(blackData) + sizeof(blackArrayData) + sizeof(whiteData) + sizeof(errorData) + sizeof(transparentData);

    auto graph = RHI::GetGraph();
    auto stage = graph->AcquireStagingBuffer(stageSize);
    {
        TextureDataRegion region;
        region.bufferOffset = 0ull;
        region.level = 0u;
        region.layer = 0u;
        region.layers = 1u;
        region.offset = PK_UINT3_ZERO;
        region.extent = { 2u, 2u, 1u };

        auto pMapped = static_cast<uint8_t*>(stage->GetMappedPtr());

        memcpy(pMapped + region.bufferOffset, blackData, sizeof(blackData)); 
        graph->CopyToTexture(QueueType::Transfer, BlackTexture2D.get(), stage, &region, 1u);
        region.bufferOffset += sizeof(blackData);

        memcpy(pMapped + region.bufferOffset, whiteData, sizeof(whiteData));
        graph->CopyToTexture(QueueType::Transfer, WhiteTexture2D.get(), stage, &region, 1u);
        region.bufferOffset += sizeof(whiteData);

        memcpy(pMapped + region.bufferOffset, errorData, sizeof(errorData));
        graph->CopyToTexture(QueueType::Transfer, ErrorTexture2D.get(), stage, &region, 1u);
        region.bufferOffset += sizeof(errorData);

        memcpy(pMapped + region.bufferOffset, transparentData, sizeof(transparentData));
        graph->CopyToTexture(QueueType::Transfer, TransparentTexture2D.get(), stage, &region, 1u);
        region.bufferOffset += sizeof(transparentData);

        region.layers = 2u;
        memcpy(pMapped + region.bufferOffset, blackArrayData, sizeof(blackArrayData));
        graph->CopyToTexture(QueueType::Transfer, BlackTexture2DArray.get(), stage, &region, 1u);
        region.bufferOffset += sizeof(blackArrayData);
    }
    graph->ReleaseStagingBuffer(stage);
}
