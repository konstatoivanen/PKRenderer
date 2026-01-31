#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "BuiltInResources.h"

PK::BuiltInResources::BuiltInResources()
{
    PK_LOG_RHI_FUNC("");

    auto commandBuffer = RHI::GetCommandBuffer(QueueType::Transfer);

    const uint32_t blackData[4] = { 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u };
    const uint32_t blackDataArray[8] = { 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u };
    const uint32_t whiteData[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    const uint32_t errorData[4] = { 0xFFFF00FFu, 0xFF000000u, 0xFF000000u, 0xFFFF00FFu };
    const uint32_t transparentData[4] = { 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu };

    TextureDataRegion region;
    region.bufferOffset = 0ull;
    region.level = 0u;
    region.layer = 0u;
    region.layers = 1u;
    region.offset = PK_UINT3_ZERO;
    region.extent = { 2u, 2u, 1u };

    TextureDescriptor textureDesc{};
    textureDesc.resolution = { 2u, 2u, 1u };
    textureDesc.usage = TextureUsage::DefaultDisk;

    BlackTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Black");
    WhiteTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.White");
    ErrorTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Error");
    TransparentTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Transparent");

    textureDesc.layers = 2;
    textureDesc.type = TextureType::Texture2DArray;
    BlackTexture2DArray = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2DArray.Black");

    commandBuffer->CopyToTexture(BlackTexture2D.get(), blackData, sizeof(blackData), &region, 1u);
    commandBuffer->CopyToTexture(WhiteTexture2D.get(), whiteData, sizeof(whiteData), &region, 1u);
    commandBuffer->CopyToTexture(ErrorTexture2D.get(), errorData, sizeof(errorData), &region, 1u);
    commandBuffer->CopyToTexture(TransparentTexture2D.get(), transparentData, sizeof(transparentData), &region, 1u);

    region.layers = 2u;
    commandBuffer->CopyToTexture(BlackTexture2DArray.get(), blackDataArray, sizeof(blackDataArray), &region, 1u);
}
