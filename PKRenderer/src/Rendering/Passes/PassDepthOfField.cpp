#include "PrecompiledHeader.h"
#include "PassDepthOfField.h"
#include "Rendering/HashCache.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Rendering::Objects;
    using namespace Rendering::Structs;

    PassDepthOfField::PassDepthOfField(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        m_shaderBlur = assetDatabase->Find<Shader>("VS_DepthOfFieldBlur");
        m_shaderComposite = assetDatabase->Find<Shader>("VS_DepthOfFieldComposite");
        m_computeAutoFocus = assetDatabase->Find<Shader>("CS_AutoFocus");

        m_constants.pk_FocalLength = config->CameraFocalLength;
        m_constants.pk_FNumber = config->CameraFNumber;
        m_constants.pk_FilmHeight = config->CameraFilmHeight;
        m_constants.pk_FocusSpeed = config->CameraFocusSpeed;

        TextureDescriptor descriptor{};
        descriptor.samplerType = SamplerType::Sampler2D;
        descriptor.format = TextureFormat::RGBA16F;
        descriptor.resolution.x = config->InitialWidth / 2;
        descriptor.resolution.y = config->InitialHeight / 2;
        descriptor.layers = 3;
        descriptor.usage = TextureUsage::RTColorSample;
        descriptor.sampler.filter = FilterMode::Bilinear;
        m_renderTarget = Texture::Create(descriptor, "Depth Of Field Texture");
        m_autoFocusParams = Buffer::CreateStorage(BufferLayout({ {ElementType::Float2, "PARAMS"} }), 1, BufferUsage::None, "Auto Focus Parameters");

        m_passPrefilter = m_shaderBlur->GetVariantIndex(StringHashID::StringToID("PASS_PREFILTER"));
        m_passDiskblur = m_shaderBlur->GetVariantIndex(StringHashID::StringToID("PASS_DISKBLUR"));
    }

    void PassDepthOfField::Execute(RenderTexture* destination, MemoryAccessFlags lastAccess)
    {
        auto hash = HashCache::Get();
        auto cmd = GraphicsAPI::GetCommandBuffer();
        auto autoFocusParams = m_autoFocusParams.get();
        auto renderTarget = m_renderTarget.get();
        auto source = destination->GetColor(0);

        auto res = destination->GetResolution();
        res.x /= 2;
        res.y /= 2;

        renderTarget->Validate(res);

        m_constants.pk_MaximumCoC = std::min(0.05f, 10.0f / destination->GetResolution().y);
        cmd->SetConstant<Constants>(hash->pk_DofParams, m_constants);

        cmd->SetBuffer(hash->pk_AutoFocusParams, autoFocusParams);
        cmd->Dispatch(m_computeAutoFocus, 0, { 1u, 1u, 1u });
        cmd->Barrier(autoFocusParams, MemoryAccessFlags::ComputeReadWrite, MemoryAccessFlags::StageFragment | MemoryAccessFlags::ReadShader);
        
        cmd->Barrier(source, lastAccess, MemoryAccessFlags::FragmentTexture);

        cmd->SetTexture(hash->_MainTex, source);
        cmd->SetRenderTarget(renderTarget, 0, 2);
        cmd->DiscardColor(0u);
        cmd->SetViewPort(renderTarget->GetRect());
        cmd->SetScissor(renderTarget->GetRect());
        cmd->Blit(m_shaderBlur, m_passPrefilter);
        
        cmd->SetTexture(hash->_MainTex, renderTarget, 0, 2);

        cmd->SetRenderTarget(renderTarget, { { 0u, 0u, 1u, 1u}, { 0u, 1u, 1u, 1u} });
        cmd->DiscardColor(0u);
        cmd->DiscardColor(1u);
        cmd->Blit(m_shaderBlur, m_passDiskblur);

        cmd->SetTexture(hash->pk_Foreground, renderTarget, 0, 0);
        cmd->SetTexture(hash->pk_Background, renderTarget, 0, 1);
        cmd->SetRenderTarget(source);
        cmd->SetViewPort(source->GetRect());
        cmd->SetScissor(source->GetRect());
        cmd->Blit(m_shaderComposite, 0);
    }

    void PassDepthOfField::OnUpdateParameters(const ApplicationConfig* config)
    {
        m_constants.pk_FocalLength = config->CameraFocalLength;
        m_constants.pk_FNumber = config->CameraFNumber;
        m_constants.pk_FilmHeight = config->CameraFilmHeight;
        m_constants.pk_FocusSpeed = config->CameraFocusSpeed;
    }
}
