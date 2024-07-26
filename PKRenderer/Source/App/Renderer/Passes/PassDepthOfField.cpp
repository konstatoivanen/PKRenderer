#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassDepthOfField.h"

namespace PK::App
{
    PassDepthOfField::PassDepthOfField(AssetDatabase* assetDatabase, const uint2& initialResolution)
    {
        PK_LOG_VERBOSE("PassDepthOfField.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_computeDepthOfField = assetDatabase->Find<ShaderAsset>("CS_DepthOfField");
        m_computeAutoFocus = assetDatabase->Find<ShaderAsset>("CS_AutoFocus");
        m_passPrefilter = m_computeDepthOfField->GetRHIIndex("PASS_PREFILTER");
        m_passDiskblur = m_computeDepthOfField->GetRHIIndex("PASS_DISKBLUR");
        m_passUpsample = m_computeDepthOfField->GetRHIIndex("PASS_UPSAMPLE");

        TextureDescriptor descriptor{};
        descriptor.type = TextureType::Texture2DArray;
        descriptor.format = TextureFormat::RGB9E5;
        descriptor.formatAlias = TextureFormat::R32UI;
        descriptor.resolution = { initialResolution / 2u, 1u };
        descriptor.layers = 3;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        m_colorTarget = RHI::CreateTexture(descriptor, "DepthOfField.Target.Color");

        descriptor.format = TextureFormat::R16F;
        descriptor.formatAlias = TextureFormat::Invalid;
        descriptor.layers = 2;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        m_alphaTarget = RHI::CreateTexture(descriptor, "DepthOfField.Target.Alpha");

        m_autoFocusParams = RHI::CreateBuffer<float2>(1ull, BufferUsage::DefaultStorage, "DepthOfField.AutoFocus.Parameters");
        RHI::SetBuffer(HashCache::Get()->pk_DoF_AutoFocusParams, m_autoFocusParams.get());
    }

    void PassDepthOfField::SetViewConstants(RenderView* view)
    {
        auto& settings = view->settings.DepthOfFieldSettings;
        m_constants.pk_DoF_FocalLength = settings.FocalLength;
        m_constants.pk_DoF_FNumber = settings.FNumber;
        m_constants.pk_DoF_FilmHeight = settings.FilmHeight;
        m_constants.pk_DoF_FocusSpeed = settings.FocusSpeed;
    }

    void PassDepthOfField::ComputeAutoFocus(CommandBufferExt cmd, uint32_t screenHeight)
    {
        m_constants.pk_DoF_MaximumCoC = std::min(0.05f, 10.0f / screenHeight);
        RHI::SetConstant<Constants>(HashCache::Get()->pk_DoF_Params, m_constants);
        cmd.Dispatch(m_computeAutoFocus, 0, { 1u, 1u, 1u });
    }

    void PassDepthOfField::Render(CommandBufferExt cmd, RHITexture* destination)
    {
        cmd->BeginDebugScope("DepthOfField", PK_COLOR_MAGENTA);

        auto fullres = destination->GetResolution();
        auto quarterres = uint3(fullres.x / 2, fullres.y / 2, 1u);

        RHI::ValidateTexture(m_colorTarget, quarterres);
        RHI::ValidateTexture(m_alphaTarget, quarterres);

        m_constants.pk_DoF_MaximumCoC = std::min(0.05f, 10.0f / destination->GetResolution().y);

        auto hash = HashCache::Get();
        RHI::SetConstant<Constants>(hash->pk_DoF_Params, m_constants);
        RHI::SetImage(hash->pk_DoF_ColorWrite, m_colorTarget.get());
        RHI::SetImage(hash->pk_DoF_AlphaWrite, m_alphaTarget.get());
        RHI::SetTexture(hash->pk_DoF_ColorRead, m_colorTarget.get());
        RHI::SetTexture(hash->pk_DoF_AlphaRead, m_alphaTarget.get());

        RHI::SetTexture(hash->pk_Texture, destination); // Prefilter Source
        RHI::SetImage(hash->pk_Image, destination); // Upsample Dest

        cmd.Dispatch(m_computeDepthOfField, m_passPrefilter, quarterres);
        cmd.Dispatch(m_computeDepthOfField, m_passDiskblur, quarterres);
        cmd.Dispatch(m_computeDepthOfField, m_passUpsample, fullres);

        cmd->EndDebugScope();
    }
}
