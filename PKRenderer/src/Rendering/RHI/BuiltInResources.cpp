#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "BuiltInResources.h"

PK::Rendering::RHI::BuiltInResources::BuiltInResources()
{
    PK_LOG_RHI("PK::Rendering::RHI::BuiltInResources.Ctor");
    PK_LOG_SCOPE_INDENT(local);

    auto cmd = RHIGetCommandBuffer(QueueType::Transfer);

    const uint32_t blackData[4] = { 0xFF000000u, 0xFF000000u, 0xFF000000u, 0xFF000000u };
    const uint32_t whiteData[4] = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
    const uint32_t transparentData[4] = { 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu, 0x00FFFFFFu };

    TextureDescriptor textureDesc{};
    textureDesc.resolution = { 2u, 2u, 1u };
    textureDesc.usage = TextureUsage::DefaultDisk;

    BlackTexture2D = RHICreateTexture(textureDesc, "PKBuiltIn.Texture2D.Black");
    WhiteTexture2D = RHICreateTexture(textureDesc, "PKBuiltIn.Texture2D.White");
    TransparentTexture2D = RHICreateTexture(textureDesc, "PKBuiltIn.Texture2D.Transparent");
    cmd->UploadTexture(BlackTexture2D.get(), blackData, sizeof(blackData), 0u, 0u);
    cmd->UploadTexture(WhiteTexture2D.get(), whiteData, sizeof(whiteData), 0u, 0u);
    cmd->UploadTexture(TransparentTexture2D.get(), transparentData, sizeof(transparentData), 0u, 0u);

    AtomicCounter = RHICreateBuffer<uint32_t>(1ull, BufferUsage::DefaultStorage, "PKBuiltIn.AtomicCounter");
    RHISetBuffer(PK::Assets::Shader::PK_SHADER_ATOMIC_COUNTER, AtomicCounter.get());
}
