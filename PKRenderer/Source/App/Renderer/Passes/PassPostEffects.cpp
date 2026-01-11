#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsColor.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "Core/Rendering/TextureAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassPostEffects.h"

namespace PK::App
{
    PassPostEffectsComposite::PassPostEffectsComposite(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");
        m_computeComposite = assetDatabase->Find<ShaderAsset>("CS_PostEffectsComposite").get();
    }

    void PassPostEffectsComposite::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& colorGrading = view->settings.ColorGradingSettings;
        auto& vignette = view->settings.VignetteSettings;
        auto& features = view->settings.PostEffectSettings;
        auto& debug = view->settings.RenderingDebugSettings;

        auto newCCLut = colorGrading.LutTextureAsset != nullptr ? colorGrading.LutTextureAsset->GetRHI() : nullptr;

        if (m_colorgradingLut != newCCLut && newCCLut != nullptr)
        {
            m_colorgradingLut = newCCLut;
            auto smp = m_colorgradingLut->GetSamplerDescriptor();
            smp.wrap[0] = WrapMode::Clamp;
            smp.wrap[1] = WrapMode::Clamp;
            smp.wrap[2] = WrapMode::Clamp;
            smp.filterMin = FilterMode::Trilinear;
            smp.filterMag = FilterMode::Trilinear;
            m_colorgradingLut->SetSampler(smp);
            RHI::SetTexture(hash->pk_CC_LutTex, m_colorgradingLut);
        }

        auto newTonemapLut = colorGrading.TonemapLutTextureAsset != nullptr ? colorGrading.TonemapLutTextureAsset->GetRHI() : nullptr;

        if (m_colorgradingLut != newTonemapLut && newTonemapLut != nullptr)
        {
            m_tonemappingLut = newTonemapLut;
            auto smp = m_tonemappingLut->GetSamplerDescriptor();
            smp.wrap[0] = WrapMode::Clamp;
            smp.wrap[1] = WrapMode::Clamp;
            smp.wrap[2] = WrapMode::Clamp;
            smp.filterMin = FilterMode::Trilinear;
            smp.filterMag = FilterMode::Trilinear;
            m_tonemappingLut->SetSampler(smp);
            RHI::SetTexture(hash->pk_Tonemap_LutTex, m_tonemappingLut);
        }

        color lift, gamma, gain;
        Math::GenerateLiftGammaGain(Math::HexToRGB(colorGrading.Shadows), Math::HexToRGB(colorGrading.Midtones), Math::HexToRGB(colorGrading.Highlights), &lift, &gamma, &gain);
        view->constants->Set<float4>(hash->pk_CC_WhiteBalance, Math::GetWhiteBalance(colorGrading.TemperatureShift, colorGrading.Tint));
        view->constants->Set<float4>(hash->pk_CC_Lift, lift);
        view->constants->Set<float4>(hash->pk_CC_Gamma, gamma);
        view->constants->Set<float4>(hash->pk_CC_Gain, gain);
        view->constants->Set<float4>(hash->pk_CC_HSV, float4((float)colorGrading.Hue, (float)colorGrading.Saturation, (float)colorGrading.Value, 1.0f));
        view->constants->Set<float4>(hash->pk_CC_MixRed, Math::HexToRGB(colorGrading.ChannelMixerRed));
        view->constants->Set<float4>(hash->pk_CC_MixGreen, Math::HexToRGB(colorGrading.ChannelMixerGreen));
        view->constants->Set<float4>(hash->pk_CC_MixBlue, Math::HexToRGB(colorGrading.ChannelMixerBlue));
        view->constants->Set<float>(hash->pk_CC_LumaContrast, colorGrading.Contrast);
        view->constants->Set<float>(hash->pk_CC_LumaGain, colorGrading.Gain);
        view->constants->Set<float>(hash->pk_CC_LumaGamma, 1.0f / colorGrading.Gamma);
        view->constants->Set<float>(hash->pk_CC_Vibrance, colorGrading.Vibrance);
        view->constants->Set<float>(hash->pk_CC_Contribution, colorGrading.Contribution);

        view->constants->Set<float>(hash->pk_Vignette_Intensity, vignette.Intensity);
        view->constants->Set<float>(hash->pk_Vignette_Power, vignette.Power);

        uint featureMask = 0u;
        featureMask |= (uint)(features.Vignette) << 0;
        featureMask |= (uint)(features.Bloom) << 1;
        featureMask |= (uint)(features.Tonemap) << 2;
        featureMask |= (uint)(features.Filmgrain) << 3;
        featureMask |= (uint)(features.Colorgrading) << 4;
        featureMask |= (uint)(features.LUTColorGrading && m_colorgradingLut != nullptr) << 5;

        featureMask |= (uint)(debug.GIDiff) << 6;
        featureMask |= (uint)(debug.GISpec) << 7;
        featureMask |= (uint)(debug.GIVX) << 8;
        featureMask |= (uint)(debug.Normal) << 9;
        featureMask |= (uint)(debug.Roughness) << 10;
        featureMask |= (uint)(debug.LightTiles) << 11;
        featureMask |= (uint)(debug.HalfScreen) << 12;
        featureMask |= (uint)(debug.Zoom) << 13;

        view->constants->Set<uint>(hash->pk_PostEffectsFeatureMask, featureMask);

        // All but lut regular color grading
        const uint fullFeatureMask = 0x2Fu;
        const bool useFullFeaturePass = (featureMask & fullFeatureMask) == fullFeatureMask;

        // Half screen & debug zoom are additive features;
        const uint debugFeatureMask = 0xFC0u;
        const bool useDebugPass = (featureMask & debugFeatureMask) != 0u;

        m_passIndex = useFullFeaturePass ? 0u : 1u;
        m_passIndex = useDebugPass ? 2u : m_passIndex;
    }

    void PassPostEffectsComposite::Render(CommandBufferExt cmd, RHITexture* destination)
    {
        auto resolution = destination->GetResolution();
        cmd->BeginDebugScope("PostEffects.Composite", PK_COLOR_YELLOW);
        RHI::SetImage(HashCache::Get()->pk_Image, destination, 0, 0);
        cmd.Dispatch(m_computeComposite, m_passIndex, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();
    }
}
