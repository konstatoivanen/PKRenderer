#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/ApplicationConfig.h"
#include "Graphics/RHI/RHIBuffer.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/Shader.h"
#include "Renderer/HashCache.h"
#include "PassDepthOfField.h"

namespace PK::Renderer::Passes
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

    PassDepthOfField::PassDepthOfField(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        PK_LOG_VERBOSE("PassDepthOfField.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeDepthOfField = assetDatabase->Find<Shader>("CS_DepthOfField");
        m_computeAutoFocus = assetDatabase->Find<Shader>("CS_AutoFocus");
        m_passPrefilter = m_computeDepthOfField->GetRHIIndex("PASS_PREFILTER");
        m_passDiskblur = m_computeDepthOfField->GetRHIIndex("PASS_DISKBLUR");
        m_passUpsample = m_computeDepthOfField->GetRHIIndex("PASS_UPSAMPLE");

        OnUpdateParameters(config);

        TextureDescriptor descriptor{};
        descriptor.samplerType = SamplerType::Sampler2DArray;
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        descriptor.resolution.x = config->InitialWidth / 2;
        descriptor.resolution.y = config->InitialHeight / 2;
        descriptor.layers = 3;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        m_colorTarget = RHICreateTexture(descriptor, "DepthOfField.Target.Color");

        descriptor.format = TextureFormat::R16F;
        descriptor.formatAlias = TextureFormat::Invalid;
        descriptor.layers = 2;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_alphaTarget = RHICreateTexture(descriptor, "DepthOfField.Target.Alpha");

        m_autoFocusParams = RHICreateBuffer<float2>(1ull, BufferUsage::DefaultStorage, "DepthOfField.AutoFocus.Parameters");
        RHISetBuffer(HashCache::Get()->pk_DoF_AutoFocusParams, m_autoFocusParams.get());
    }

    void PassDepthOfField::ComputeAutoFocus(CommandBufferExt cmd, uint32_t screenHeight)
    {
        m_constants.pk_DoF_MaximumCoC = std::min(0.05f, 10.0f / screenHeight);
        RHISetConstant<Constants>(HashCache::Get()->pk_DoF_Params, m_constants);
        cmd.Dispatch(m_computeAutoFocus, 0, { 1u, 1u, 1u });
    }

    void PassDepthOfField::Render(CommandBufferExt cmd, Texture* destination)
    {
        cmd->BeginDebugScope("DepthOfField", Math::PK_COLOR_MAGENTA);

        auto fullres = destination->GetResolution();
        auto quarterres = Math::uint3(fullres.x / 2, fullres.y / 2, 1u);

        m_colorTarget->Validate(quarterres);
        m_alphaTarget->Validate(quarterres);

        m_constants.pk_DoF_MaximumCoC = std::min(0.05f, 10.0f / destination->GetResolution().y);

        auto hash = HashCache::Get();
        RHISetConstant<Constants>(hash->pk_DoF_Params, m_constants);
        RHISetImage(hash->pk_DoF_ColorWrite, m_colorTarget.get());
        RHISetImage(hash->pk_DoF_AlphaWrite, m_alphaTarget.get());
        RHISetTexture(hash->pk_DoF_ColorRead, m_colorTarget.get());
        RHISetTexture(hash->pk_DoF_AlphaRead, m_alphaTarget.get());

        RHISetTexture(hash->pk_Texture, destination); // Prefilter Source
        RHISetImage(hash->pk_Image, destination); // Upsample Dest

        cmd.Dispatch(m_computeDepthOfField, m_passPrefilter, quarterres);
        cmd.Dispatch(m_computeDepthOfField, m_passDiskblur, quarterres);
        cmd.Dispatch(m_computeDepthOfField, m_passUpsample, fullres);

        cmd->EndDebugScope();
    }

    void PassDepthOfField::OnUpdateParameters(const ApplicationConfig* config)
    {
        m_constants.pk_DoF_FocalLength = config->DoFFocalLength;
        m_constants.pk_DoF_FNumber = config->DoFFNumber;
        m_constants.pk_DoF_FilmHeight = config->DoFFilmHeight;
        m_constants.pk_DoF_FocusSpeed = config->DoFFocusSpeed;
    }
}
