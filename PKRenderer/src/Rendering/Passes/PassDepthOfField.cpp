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
        m_computeDepthOfField = assetDatabase->Find<Shader>("CS_DepthOfField");
        m_computeAutoFocus = assetDatabase->Find<Shader>("CS_AutoFocus");
        m_passPrefilter = m_computeDepthOfField->GetVariantIndex(StringHashID::StringToID("PASS_PREFILTER"));
        m_passDiskblur = m_computeDepthOfField->GetVariantIndex(StringHashID::StringToID("PASS_DISKBLUR"));
        m_passUpsample = m_computeDepthOfField->GetVariantIndex(StringHashID::StringToID("PASS_UPSAMPLE"));

        m_constants.pk_FocalLength = config->CameraFocalLength;
        m_constants.pk_FNumber = config->CameraFNumber;
        m_constants.pk_FilmHeight = config->CameraFilmHeight;
        m_constants.pk_FocusSpeed = config->CameraFocusSpeed;

        TextureDescriptor descriptor{};
        descriptor.samplerType = SamplerType::Sampler2DArray;
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.resolution.x = config->InitialWidth / 2;
        descriptor.resolution.y = config->InitialHeight / 2;
        descriptor.layers = 3;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage | TextureUsage::Aliased;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        m_colorTarget = Texture::Create(descriptor, "DepthOfField.Target.Color");

        descriptor.format = TextureFormat::R16F;
        descriptor.layers = 2;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_alphaTarget = Texture::Create(descriptor, "DepthOfField.Target.Alpha");

        m_autoFocusParams = Buffer::Create(ElementType::Float2, 1, BufferUsage::DefaultStorage, "DepthOfField.AutoFocus.Parameters");
        GraphicsAPI::SetBuffer(HashCache::Get()->pk_AutoFocusParams, m_autoFocusParams.get());
    }

    void PassDepthOfField::ComputeAutoFocus(Objects::CommandBuffer* cmd, uint32_t screenHeight)
    {
        m_constants.pk_MaximumCoC = std::min(0.05f, 10.0f / screenHeight);
        GraphicsAPI::SetConstant<Constants>(HashCache::Get()->pk_DofParams, m_constants);
        cmd->Dispatch(m_computeAutoFocus, 0, { 1u, 1u, 1u });
    }

    void PassDepthOfField::Render(Objects::CommandBuffer* cmd, RenderTexture* destination)
    {
        cmd->BeginDebugScope("DepthOfField", Math::PK_COLOR_MAGENTA);

        auto colorTarget = m_colorTarget.get();
        auto alphaTarget = m_colorTarget.get();

        auto source = destination->GetColor(0);
        auto fullres = destination->GetResolution();
        auto quarterres = Math::uint3(fullres.x / 2, fullres.y / 2, 1u);

        m_colorTarget->Validate(quarterres);
        m_alphaTarget->Validate(quarterres);

        m_constants.pk_MaximumCoC = std::min(0.05f, 10.0f / destination->GetResolution().y);
        
        auto hash = HashCache::Get();
        GraphicsAPI::SetConstant<Constants>(hash->pk_DofParams, m_constants);
        GraphicsAPI::SetImage(hash->pk_DoFColorWrite, m_colorTarget.get());
        GraphicsAPI::SetImage(hash->pk_DoFAlphaWrite, m_alphaTarget.get());
        GraphicsAPI::SetTexture(hash->pk_DoFColorRead, m_colorTarget.get());
        GraphicsAPI::SetTexture(hash->pk_DoFAlphaRead, m_alphaTarget.get());
        GraphicsAPI::SetImage(hash->_DestinationTex, source);
        GraphicsAPI::SetTexture(hash->_MainTex, source);
        
        cmd->Dispatch(m_computeDepthOfField, m_passPrefilter, quarterres);
        cmd->Dispatch(m_computeDepthOfField, m_passDiskblur, quarterres);
        cmd->Dispatch(m_computeDepthOfField, m_passUpsample, fullres);
        
        cmd->EndDebugScope();
    }

    void PassDepthOfField::OnUpdateParameters(const ApplicationConfig* config)
    {
        m_constants.pk_FocalLength = config->CameraFocalLength;
        m_constants.pk_FNumber = config->CameraFNumber;
        m_constants.pk_FilmHeight = config->CameraFilmHeight;
        m_constants.pk_FocusSpeed = config->CameraFocusSpeed;
    }
}
