#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "BuiltInResources.h"

PK::BuiltInResources::BuiltInResources()
{
    PK_LOG_RHI("PK::BuiltInResources.Ctor");
    PK_LOG_SCOPE_INDENT(local);

    auto commandBuffer = RHI::GetCommandBuffer(QueueType::Transfer);

    const uint32_t blackData[4] = { 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u };
    const uint32_t whiteData[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    const uint32_t transparentData[4] = { 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu };

    TextureDescriptor textureDesc{};
    textureDesc.resolution = { 2u, 2u, 1u };
    textureDesc.usage = TextureUsage::DefaultDisk;

    BlackTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Black");
    WhiteTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.White");
    TransparentTexture2D = RHI::CreateTexture(textureDesc, "PKBuiltIn.Texture2D.Transparent");
    commandBuffer->UploadTexture(BlackTexture2D.get(), blackData, sizeof(blackData), 0u, 0u);
    commandBuffer->UploadTexture(WhiteTexture2D.get(), whiteData, sizeof(whiteData), 0u, 0u);
    commandBuffer->UploadTexture(TransparentTexture2D.get(), transparentData, sizeof(transparentData), 0u, 0u);

    AtomicCounter = RHI::CreateBuffer(sizeof(uint32_t), BufferUsage::DefaultStorage, "PKBuiltIn.AtomicCounter");
    RHI::SetBuffer(PK::Assets::Shader::PK_SHADER_ATOMIC_COUNTER, AtomicCounter.get());
}
