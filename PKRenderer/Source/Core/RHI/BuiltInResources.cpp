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

    TextureUploadRange uploadRange;
    uploadRange.bufferOffset = 0ull;
    uploadRange.level = 0u;
    uploadRange.layer = 0u;
    uploadRange.layers = 1u;
    uploadRange.offset = PK_UINT3_ZERO;
    uploadRange.extent = { 2u, 2u, 1u };

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

    commandBuffer->UploadTexture(BlackTexture2D.get(), blackData, sizeof(blackData), &uploadRange, 1u);
    commandBuffer->UploadTexture(WhiteTexture2D.get(), whiteData, sizeof(whiteData), &uploadRange, 1u);
    commandBuffer->UploadTexture(ErrorTexture2D.get(), errorData, sizeof(errorData), &uploadRange, 1u);
    commandBuffer->UploadTexture(TransparentTexture2D.get(), transparentData, sizeof(transparentData), &uploadRange, 1u);

    uploadRange.layers = 2u;
    commandBuffer->UploadTexture(BlackTexture2DArray.get(), blackDataArray, sizeof(blackDataArray), &uploadRange, 1u);

    AtomicCounter = RHI::CreateBuffer(sizeof(uint32_t), BufferUsage::DefaultStorage, "PKBuiltIn.AtomicCounter");
    RHI::SetBuffer(PKAssets::PK_SHADER_ATOMIC_COUNTER, AtomicCounter.get());
}
